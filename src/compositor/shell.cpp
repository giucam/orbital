/*
 * Copyright 2013-2014 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <signal.h>
#include <linux/input.h>
#include <sys/resource.h>

#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QSettings>

#include "shell.h"
#include "compositor.h"
#include "layer.h"
#include "workspace.h"
#include "shellsurface.h"
#include "seat.h"
#include "binding.h"
#include "view.h"
#include "shellview.h"
#include "output.h"
#include "xwayland.h"
#include "global.h"
#include "pager.h"
#include "dropdown.h"
#include "screenshooter.h"
#include "focusscope.h"
#include "clipboard.h"
#include "dashboard.h"
#include "gammacontrol.h"
#include "wlshell/wlshell.h"
#include "desktop-shell/desktop-shell.h"
#include "desktop-shell/desktop-shell-workspace.h"
#include "desktop-shell/desktop-shell-window.h"
#include "effects/zoomeffect.h"
#include "effects/desktopgrid.h"

namespace Orbital {

Shell::Shell(Compositor *c)
     : Object()
     , m_compositor(c)
     , m_grabCursorSetter(nullptr)
     , m_grabCursorUnsetter(nullptr)
     , m_pager(new Pager(c))
     , m_locked(false)
     , m_lockScope(new FocusScope(this))
     , m_appsScope(new FocusScope(this))
{
    initEnvironment();

    addInterface(new XWayland(this));
    addInterface(new WlShell(this, m_compositor));
    addInterface(new DesktopShell(this));
    addInterface(new Dropdown(this));
    addInterface(new Screenshooter(this));
    addInterface(new ClipboardManager(this));
    addInterface(new GammaControlManager(this));

    new ZoomEffect(this);
    new DesktopGrid(this);
    new Dashboard(this);

    foreach (Seat *s, m_compositor->seats()) {
        s->activate(m_appsScope);
    }

    m_focusBinding = c->createButtonBinding(PointerButton::Left, KeyboardModifiers::None);
    m_raiseBinding = c->createButtonBinding(PointerButton::Task, KeyboardModifiers::None);
    m_moveBinding = c->createButtonBinding(PointerButton::Left, KeyboardModifiers::Super);
    m_killBinding = c->createKeyBinding(KEY_ESC, KeyboardModifiers::Super | KeyboardModifiers::Ctrl);
    addAction("ActivateNextWorkspace", [this](Seat *s) { nextWs(s); });
    addAction("ActivatePreviousWorkspace", [this](Seat *s) { prevWs(s); });
    m_alphaBinding = c->createAxisBinding(PointerAxis::Vertical, KeyboardModifiers::Ctrl);
    connect(m_focusBinding, &ButtonBinding::triggered, this, &Shell::giveFocus);
    connect(m_raiseBinding, &ButtonBinding::triggered, this, &Shell::raise);
    connect(m_moveBinding, &ButtonBinding::triggered, this, &Shell::moveSurface);
    connect(m_killBinding, &KeyBinding::triggered, this, &Shell::killSurface);
    connect(m_alphaBinding, &AxisBinding::triggered, this, &Shell::setAlpha);

    autostartClients();
}

Shell::~Shell()
{
    qDeleteAll(m_surfaces);
    foreach (Workspace *w, m_workspaces) {
        delete w;
    }
    delete m_pager;
    delete m_focusBinding;
    delete m_raiseBinding;
    delete m_moveBinding;
}

void Shell::initEnvironment()
{
    setenv("QT_QPA_PLATFORM", "wayland", 0);

    if (qEnvironmentVariableIsSet("DBUS_SESSION_BUS_ADDRESS")) {
        return;
    }

    QProcess proc;
    proc.start(QStringLiteral("dbus-launch"), { QStringLiteral("--binary-syntax") });
    if (!proc.waitForStarted()) {
        qWarning("Could not start the DBus session.");
        return;
    }
    pid_t pid = proc.processId();
    proc.waitForFinished();
    QByteArray out = proc.readAllStandardOutput();

    setenv("DBUS_SESSION_BUS_ADDRESS", out.constData(), 1);
    setenv("DBUS_SESSION_BUS_PID", qPrintable(QString::number(pid)), 1);
}

static bool shouldAutoStart(const QSettings &settings)
{
    bool hidden = settings.value(QStringLiteral("Hidden")).toBool();
    if (hidden) {
        return false;
    }
    if (settings.contains(QStringLiteral("OnlyShowIn"))) {
        QString onlyShowIn = settings.value(QStringLiteral("OnlyShowIn")).toString();
        foreach (const QStringRef &s, onlyShowIn.splitRef(QLatin1Char(';'))) {
            if (s == QLatin1String("Orbital")) {
                return true;
            }
        }
        return false;
    } else if (settings.contains(QStringLiteral("NotShowIn"))) {
        QString notShowIn = settings.value(QStringLiteral("NotShowIn")).toString();
        foreach (const QStringRef &s, notShowIn.splitRef(QLatin1Char(';'))) {
            if (s == QStringLiteral("Orbital")) {
                return false;
            }
        }
    }
    return true;
}

static void populateAutostartList(QStringList &files, const QString &autostartDir)
{
    QDir dir(autostartDir);
    if (dir.exists()) {
        QFileInfoList infos = dir.entryInfoList({ QStringLiteral("*.desktop") }, QDir::Files);
        foreach (const QFileInfo &fi, infos) {
            QString path = fi.absoluteFilePath();
            QString filename = fi.fileName();
            bool add = true;
            if (!fi.isReadable()) {
                continue;
            }

            foreach (const QString &p, files) {
                if (QFileInfo(p).fileName() == filename) {
                    add = false;
                    break;
                }
            }
            if (add) {
                files << path;
            }
        }
    }
}

static bool readDesktopFile(QIODevice &device, QSettings::SettingsMap &map)
{
    QByteArray currentGroup;

    while (!device.atEnd()) {
        const QByteArray line = device.readLine();
        int length = line.length();
        if (line.contains('\n')) {
            --length;
        }
        if (length == 0) {
            continue;
        }

        if (line.indexOf('[') == 0) {
            int idx = line.indexOf(']');
            if (idx == -1 || idx != length - 1) {
                return false;
            }
            currentGroup = line.mid(1, length - 2);
            continue;
        }

        int idx = line.indexOf('=');
        if (idx < 1) {
            return false;
        }
        QByteArray key = line.left(idx);
        QByteArray value = line.mid(idx + 1, length - idx - 1);

        map[QString::fromUtf8(currentGroup + '/' + key)] = value;
    }

    return true;
}

void Shell::autostartClients()
{
    QStringList files;

    QByteArray xdgConfigHome = qgetenv("XDG_CONFIG_HOME");
    if (!xdgConfigHome.isEmpty()) {
        populateAutostartList(files, QStringLiteral("%1/autostart").arg(QString::fromUtf8(xdgConfigHome)));
    } else {
        populateAutostartList(files, QStringLiteral("%1/.config/autostart").arg(QDir::homePath()));
    }

    QByteArray xdgConfigDirs = qgetenv("XDG_CONFIG_DIRS");
    if (!xdgConfigDirs.isEmpty()) {
        // meh, no QStringRef overload for QString::arg()
        foreach (const QString &d, QString::fromUtf8(xdgConfigDirs).split(QLatin1Char(';'))) {
            populateAutostartList(files, QStringLiteral("%1/autostart").arg(d));
        }
    } else {
        populateAutostartList(files, QStringLiteral("/etc/xdg/autostart"));
    }

    QDir outputDir = QDir::temp();
    QString dirName = QStringLiteral("orbital-%1").arg(getpid());
    outputDir.mkdir(dirName);
    outputDir.cd(dirName);

    static QSettings::Format desktopFormat = QSettings::registerFormat(QStringLiteral("desktop"), readDesktopFile, nullptr);

    foreach (const QString &fi, files) {
        QSettings settings(fi, desktopFormat);
        settings.beginGroup(QStringLiteral("Desktop Entry"));
        if (!shouldAutoStart(settings)) {
            continue;
        }

        QString exec;
        if (settings.contains(QStringLiteral("TryExec"))) {
            exec = settings.value(QStringLiteral("TryExec")).toString();
        } else {
            exec = settings.value(QStringLiteral("Exec")).toString();
        }
        qDebug("Autostarting '%s'", qPrintable(exec));

        class Process : public QProcess
        {
        public:
            Process(QObject *p) : QProcess(p) {}
            void setupChildProcess() override { setpriority(PRIO_PROCESS, getpid(), 0); }
        };

        QProcess *proc = new Process(this);
        QString bin = exec.split(QLatin1Char(' ')).first(); // no QStringRef for QDir::filePath() either
        proc->setStandardOutputFile(outputDir.filePath(bin));
        proc->setStandardErrorFile(outputDir.filePath(bin));
        proc->start(exec);

        settings.endGroup();
    }
}

Compositor *Shell::compositor() const
{
    return m_compositor;
}

Pager *Shell::pager() const
{
    return m_pager;
}

Workspace *Shell::createWorkspace()
{
    Workspace *ws = new Workspace(this, m_workspaces.count());
    ws->addInterface(new DesktopShellWorkspace(this, ws));
    m_pager->addWorkspace(ws);
    m_workspaces << ws;
    return ws;
}

ShellSurface *Shell::createShellSurface(Surface *s)
{
    ShellSurface *surf = new ShellSurface(this, s);
    surf->addInterface(new DesktopShellWindow(findInterface<DesktopShell>()));
    m_surfaces << surf;
    connect(surf, &QObject::destroyed, [this](QObject *o) { m_surfaces.removeOne(static_cast<ShellSurface *>(o)); });
    return surf;
}

QList<Workspace *> Shell::workspaces() const
{
    return m_workspaces;
}

QList<ShellSurface *> Shell::surfaces() const
{
    return m_surfaces;
}

void Shell::setGrabCursor(Pointer *pointer, PointerCursor c)
{
    if (m_grabCursorSetter) {
        m_grabCursorSetter(pointer, c);
    }
}

void Shell::unsetGrabCursor(Pointer *p)
{
    if (m_grabCursorUnsetter) {
        m_grabCursorUnsetter(p);
    }
}

Output *Shell::selectPrimaryOutput(Seat *seat)
{
    struct Out {
        Output *output;
        int vote;
    };
    QList<Out> candidates;

    foreach (Output *o, compositor()->outputs()) {
        candidates.append({ o, 0 });
    }

    Output *output = nullptr;
    if (candidates.isEmpty()) {
        return nullptr;
    } else if (candidates.size() == 1) {
        output = candidates.first().output;
    } else {
        QList<Seat *> seats;
        if (seat) {
            seats << seat;
        } else {
            seats = compositor()->seats();
        }
        for (int i = 0; i < candidates.count(); ++i) {
            Out &o = candidates[i];
            foreach (Seat *s, seats) {
                if (o.output->geometry().contains(s->pointer()->x(), s->pointer()->y())) {
                    o.vote++;
                }
            }
        }
        Out *out = nullptr;
        for (int i = 0; i < candidates.count(); ++i) {
            Out &o = candidates[i];
            if (!out || out->vote < o.vote) {
                out = &o;
            }
        }
        output = out->output;
    }
    return output;
}

void Shell::lock(const LockCallback &callback)
{
    if (m_locked) {
        return;
    }

    emit aboutToLock();
    if (m_compositor->outputs().isEmpty()) {
        m_locked = true;
        emit locked();
    } else {
        int *numOuts = new int;
        *numOuts = m_compositor->outputs().count();
        foreach (Output *o, m_compositor->outputs()) {
            o->lock([this, numOuts, callback]() {
                if (--*numOuts == 0) {
                    m_locked = true;
                    emit locked();
                    if (callback) {
                        callback();
                    }
                    delete numOuts;
                }
            });
        }
    }
    foreach (Seat *s, m_compositor->seats()) {
        s->activate(m_lockScope);
    }
}

void Shell::unlock()
{
    m_locked = false;
    foreach (Output *o, m_compositor->outputs()) {
        o->unlock();
    }
    foreach (Seat *s, m_compositor->seats()) {
        s->activate(m_appsScope);
    }
}

bool Shell::isLocked() const
{
    return m_locked;
}

bool Shell::snapPos(Output *out, QPointF &p, int snapMargin) const
{
    QRect geom = out->availableGeometry();
    double &x = p.rx();
    double &y = p.ry();
    bool snapped = false;

    if (snapMargin < 0) {
        snapMargin = 10;
    }
    if (qAbs(geom.top() - y) < snapMargin) {
        y = geom.y();
        snapped = true;
    }
    if (qAbs(geom.left() - x) < snapMargin) {
        x = geom.x();
        snapped = true;
    }
    if (qAbs(geom.bottom() - y) < snapMargin) {
        y = geom.bottom();
        snapped = true;
    }
    if (qAbs(geom.right() - x) < snapMargin) {
        x = geom.right();
        snapped = true;
    }
    return snapped;
}

void Shell::configure(ShellSurface *shsurf)
{
    if (!shsurf->surface()->isMapped() && !shsurf->workspace()) {
        Output *output = selectPrimaryOutput();
        shsurf->setWorkspace(output->currentWorkspace());

        if (isSurfaceActive(shsurf)) {
            m_appsScope->activate(shsurf->surface());
        }
    }
}

bool Shell::isSurfaceActive(ShellSurface *shsurf) const
{
    return (shsurf->type() == ShellSurface::Type::Toplevel || shsurf->type() == ShellSurface::Type::Transient) &&
            !shsurf->isInactive();
}

void Shell::setGrabCursorSetter(GrabCursorSetter s)
{
    m_grabCursorSetter = s;
}

void Shell::setGrabCursorUnsetter(GrabCursorUnsetter s)
{
    m_grabCursorUnsetter = s;
}

void Shell::giveFocus(Seat *seat)
{
    if (seat->pointer()->isGrabActive()) {
        return;
    }

    View *focus = seat->pointer()->pickActivableView();
    if (!focus) {
        return;
    }

    Surface *surf = focus->surface()->mainSurface();
    FocusScope *scope = surf->focusScope();
    (scope ? scope : m_appsScope)->activate(surf);

    // TODO: make this a proper config option
    static bool useSeparateRaise = qEnvironmentVariableIntValue("ORBITAL_SEPARATE_RAISE");
    if (!useSeparateRaise) {
        ShellSurface *shsurf = ShellSurface::fromSurface(surf);
        if (shsurf && shsurf->isFullscreen()) {
            return;
        }

        if (shsurf) {
            foreach (Output *o, compositor()->outputs()) {
                ShellView *view = shsurf->viewForOutput(o);
                view->layer()->raiseOnTop(view);
            }
        }
    }
}

void Shell::raise(Seat *seat)
{
    if (seat->pointer()->isGrabActive()) {
        return;
    }

    View *focus = seat->pointer()->focus();
    if (!focus) {
        return;
    }

    ShellSurface *shsurf = ShellSurface::fromSurface(focus->surface()->mainSurface());
    if (shsurf && shsurf->isFullscreen()) {
        return;
    }

    if (shsurf) {
        foreach (Output *o, compositor()->outputs()) {
            ShellView *view = shsurf->viewForOutput(o);
            if (view->layer()->topView() == view) {
                view->layer()->lower(view);
            } else {
                view->layer()->raiseOnTop(view);
            }
        }
    }
}

void Shell::moveSurface(Seat *seat)
{
    if (seat->pointer()->isGrabActive()) {
        return;
    }

    View *focus = m_compositor->pickView(seat->pointer()->x(), seat->pointer()->y());
    if (!focus) {
        return;
    }

    focus->surface()->mainSurface()->move(seat);
}

void Shell::killSurface(Seat *s)
{
    class KillGrab : public PointerGrab
    {
    public:
        void motion(uint32_t time, double x, double y) override
        {
            pointer()->move(x, y);
        }
        void button(uint32_t time, PointerButton button, Pointer::ButtonState state) override
        {
            View *view = compositor->pickView(pointer()->x(), pointer()->y());
            compositor->kill(view->surface());

            end();
        }
        void ended() override
        {
            delete abortBinding;
            delete this;
        }
        KeyBinding *abortBinding;
        Compositor *compositor;
    };

    KillGrab *grab = new KillGrab;
    grab->abortBinding = m_compositor->createKeyBinding(KEY_ESC, KeyboardModifiers::None);
    grab->compositor = m_compositor;
    connect(grab->abortBinding, &KeyBinding::triggered, [grab]() { grab->end(); });
    s->pointer()->setFocus(nullptr);
    grab->start(s, PointerCursor::Kill);
}

void Shell::nextWs(Seat *s)
{
    Output *o = selectPrimaryOutput(s);
    m_pager->activateNextWorkspace(o);
}

void Shell::prevWs(Seat *s)
{
    Output *o = selectPrimaryOutput(s);
    m_pager->activatePrevWorkspace(o);
}

void Shell::setAlpha(Seat *seat, uint32_t time, PointerAxis axis, double value)
{
    if (seat->pointer()->isGrabActive()) {
        return;
    }

    View *focus = m_compositor->pickView(seat->pointer()->x(), seat->pointer()->y());
    if (!focus) {
        return;
    }

    double a = focus->alpha() + value / 200.;
    focus->setAlpha(qBound(0., a, 1.));
}

void Shell::addAction(StringView name, const Action &action)
{
    m_actions.append({ name.toStdString(), action });
    emit actionAdded(name, &m_actions.last().second);
}


Shell::ActionList::ActionList(Shell *s)
                 : shell(s)
{
}

Shell::ActionList::iterator Shell::ActionList::begin()
{
    return iterator(0, shell);
}

Shell::ActionList::iterator Shell::ActionList::end()
{
    int n = shell->m_actions.size();
    return iterator(n, shell);
}

StringView Shell::ActionList::iterator::name()
{
    return shell->m_actions.at(id).first;
}

Shell::Action *Shell::ActionList::iterator::action()
{
    return &shell->m_actions[id].second;
}

}
