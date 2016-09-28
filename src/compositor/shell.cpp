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

#include <libweston-desktop.h>

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
#include "weston-desktop/wdesktop.h"
#include "desktop-shell/desktop-shell.h"
#include "desktop-shell/desktop-shell-workspace.h"
#include "desktop-shell/desktop-shell-window.h"
#include "effects/zoomeffect.h"
#include "effects/desktopgrid.h"
#include "format.h"
#include "surface.h"
#include "desktopfile.h"

namespace Orbital {

Shell::Shell(Compositor *c)
     : Object()
     , m_compositor(c)
     , m_grabCursorSetter(nullptr)
     , m_grabCursorUnsetter(nullptr)
     , m_pager(new Pager(c))
     , m_locked(false)
     , m_lockScope(std::make_unique<FocusScope>(this))
     , m_appsScope(std::make_unique<FocusScope>(this))
{
    initEnvironment();

    addInterface(new XWayland(this));
    addInterface(new WDesktop(this, m_compositor));
    addInterface(new DesktopShell(this));
    addInterface(new Dropdown(this));
    addInterface(new Screenshooter(this));
    addInterface(new ClipboardManager(this));
    addInterface(new GammaControlManager(this));

    new ZoomEffect(this);
    new DesktopGrid(this);
    new Dashboard(this);

    for (Seat *s: m_compositor->seats()) {
        s->activate(m_appsScope.get());
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
    for (Workspace *w: m_workspaces) {
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

static bool shouldAutoStart(const DesktopFile &settings)
{
    bool hidden = settings.value<bool>("Hidden");

    if (hidden) {
        return false;
    }

    if (settings.hasValue("OnlyShowIn")) {
        bool show = false;

        settings.value("OnlyShowIn").split(';', [&show](StringView s) {
            if (s == "Orbital") {
                show = true;
                return true;
            }
            return false;
        });
        return show;
    } else if (settings.hasValue("NotShowIn")) {
        bool show = true;
        settings.value("NotShowIn").split(';', [&show](StringView s) {
            if (s == "Orbital") {
                show = false;
                return true;
            }
            return false;
        });
        if (!show) {
            return false;
        }
    }
    return true;
}

static void populateAutostartList(std::vector<std::string> &files, StringView autostartDir)
{
    QDir dir(autostartDir.toQString());
    if (dir.exists()) {
        QFileInfoList infos = dir.entryInfoList({ QStringLiteral("*.desktop") }, QDir::Files);
        foreach (const QFileInfo &fi, infos) {
            QString path = fi.absoluteFilePath();
            std::string filename = fi.fileName().toStdString();
            bool add = true;
            if (!fi.isReadable()) {
                continue;
            }

            for (const std::string &p: files) {
                if (p.find(filename) != std::string::npos) {
                    add = false;
                    break;
                }
            }
            if (add) {
                files.push_back(path.toStdString());
            }
        }
    }
}

void Shell::autostartClients()
{
    std::vector<std::string> files;

    StringView xdgConfigHome = getenv("XDG_CONFIG_HOME");
    if (!xdgConfigHome.isEmpty() && !xdgConfigHome.isNull()) {
        populateAutostartList(files, fmt::format("{}/autostart", xdgConfigHome));
    } else {
        populateAutostartList(files, fmt::format("{}/.config/autostart", getenv("HOME")));
    }

    StringView xdgConfigDirs = getenv("XDG_CONFIG_DIRS");
    if (!xdgConfigDirs.isEmpty() && !xdgConfigDirs.isNull()) {
        xdgConfigDirs.split(';', [&files](StringView substr) {
            populateAutostartList(files, fmt::format("{}/autostart", substr));
            return false;
        });
    } else {
        populateAutostartList(files, "/etc/xdg/autostart");
    }

    QDir outputDir = QDir::temp();
    QString dirName = QStringLiteral("orbital-%1").arg(getpid());
    outputDir.mkdir(dirName);
    outputDir.cd(dirName);

    for (const std::string &fi: files) {
        DesktopFile file(fi);
        if (!file.isValid()) {
            continue;
        }

        file.beginGroup("Desktop Entry");
        if (!shouldAutoStart(file)) {
            continue;
        }

        StringView exec;
        if (file.hasValue("TryExec")) {
            exec = file.value("TryExec");
        } else if (file.hasValue("Exec")) {
            exec = file.value("Exec");
        }
        fmt::print(stderr, "Autostarting {}: '{}'\n", fi, exec);

        if (exec.isEmpty()) {
            continue;
        }

        class Process : public QProcess
        {
        public:
            Process(QObject *p) : QProcess(p) {}
            void setupChildProcess() override { setpriority(PRIO_PROCESS, getpid(), 0); }
        };

        QProcess *proc = new Process(this);
        StringView binView(exec);
        StringView(exec).split(' ', [&binView](StringView substr) {
            binView = substr;
            return true;
        });
        QString bin = binView.toQString();
        proc->setStandardOutputFile(outputDir.filePath(bin));
        proc->setStandardErrorFile(outputDir.filePath(bin));
        proc->start(exec.toQString());
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
    Workspace *ws = new Workspace(this, m_workspaces.size());
    ws->addInterface(new DesktopShellWorkspace(this, ws));
    m_pager->addWorkspace(ws);
    m_workspaces.push_back(ws);
    return ws;
}

ShellSurface *Shell::createShellSurface(Surface *s, ShellSurface::Handler h)
{
    ShellSurface *surf = new ShellSurface(this, s, std::move(h));
    surf->addInterface(new DesktopShellWindow(findInterface<DesktopShell>()));
    m_surfaces.push_back(surf);
    connect(surf, &QObject::destroyed, [this](QObject *o) {
        auto it = std::find(m_surfaces.begin(), m_surfaces.end(), static_cast<ShellSurface *>(o));
        if (it != m_surfaces.end()) {
            m_surfaces.erase(it);
        }
    });
    emit shellSurfaceCreated(surf);
    return surf;
}

const std::vector<Workspace *> &Shell::workspaces() const
{
    return m_workspaces;
}

const std::vector<ShellSurface *> &Shell::surfaces() const
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
    std::vector<Out> candidates;

    for (Output *o: compositor()->outputs()) {
        candidates.push_back({ o, 0 });
    }

    Output *output = nullptr;
    if (candidates.empty()) {
        return nullptr;
    } else if (candidates.size() == 1) {
        output = candidates.front().output;
    } else {
        std::vector<Seat *> seats = [this, seat]() {
            if (seat) {
                return std::vector<Seat *>{ seat };
            } else {
                return compositor()->seats();
            }
        }();
        for (Out &o: candidates) {
            for (Seat *s: seats) {
                if (o.output->geometry().contains(s->pointer()->x(), s->pointer()->y())) {
                    o.vote++;
                }
            }
        }
        Out *out = nullptr;
        for (Out &o: candidates) {
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
    if (m_compositor->outputs().empty()) {
        m_locked = true;
        emit locked();
    } else {
        int *numOuts = new int;
        *numOuts = m_compositor->outputs().size();
        for (Output *o: m_compositor->outputs()) {
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
    for (Seat *s: m_compositor->seats()) {
        s->activate(m_lockScope.get());
    }
}

void Shell::unlock()
{
    m_locked = false;
    for (Output *o: m_compositor->outputs()) {
        o->unlock();
    }
    for (Seat *s: m_compositor->seats()) {
        s->activate(m_appsScope.get());
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
    (scope ? scope : m_appsScope.get())->activate(surf);

    // TODO: make this a proper config option
    static bool useSeparateRaise = qEnvironmentVariableIntValue("ORBITAL_SEPARATE_RAISE");
    if (!useSeparateRaise) {
        ShellSurface *shsurf = surf->shellSurface();
        if (shsurf && shsurf->isFullscreen()) {
            return;
        }

        if (shsurf) {
            for (Output *o: compositor()->outputs()) {
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

    ShellSurface *shsurf = focus->surface()->mainSurface()->shellSurface();
    if (shsurf && shsurf->isFullscreen()) {
        return;
    }

    if (shsurf) {
        for (Output *o: compositor()->outputs()) {
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
        void motion(uint32_t time, Pointer::MotionEvent evt) override
        {
            pointer()->move(evt);
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
    m_actions.emplace_back(name.toStdString(), action);
    emit actionAdded(name, &m_actions.back().second);
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
