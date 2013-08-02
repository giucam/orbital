
#include <QtQml>

#include <filebrowser.h>

static const int a = qmlRegisterType<FileBrowser>("Orbital", 1, 0, "FileBrowser");
static const int b = qmlRegisterType<FileInfo>();

FileInfo::FileInfo(const QString &path)
        : m_info(path)
{
}

QString FileInfo::name() const
{
    return m_info.fileName();
}

bool FileInfo::isDir() const
{
    return m_info.isDir();
}

QString FileInfo::path() const
{
    return m_info.filePath();
}


FileBrowser::FileBrowser(QObject *p)
           : QObject(p)
{
}

void FileBrowser::setPath(const QString &path)
{
    QString old = m_dir.absolutePath();

    QFileInfo info(path);
    if (info.isDir()) {
        m_dir.setPath(path);
    } else {
        m_dir.setPath(info.path());
    }
    if (old != m_dir.absolutePath()) {
        rebuildFilesList();
        emit pathChanged();
    }
}

void FileBrowser::setNameFilters(const QStringList &filters)
{
    m_dir.setNameFilters(filters);
}

void FileBrowser::cdUp()
{
    m_dir.cdUp();
    rebuildFilesList();
    emit pathChanged();
}

void FileBrowser::cd(const QString &dir)
{
    m_dir.cd(dir);
    rebuildFilesList();
    emit pathChanged();
}

QString FileBrowser::path() const
{
    return m_dir.path();
}

QStringList FileBrowser::nameFilters() const
{
    return m_dir.nameFilters();
}

int FileBrowser::filesCount(QQmlListProperty<FileInfo> *prop)
{
    FileBrowser *c = static_cast<FileBrowser *>(prop->object);
    return c->m_files.count();
}

FileInfo *FileBrowser::fileAt(QQmlListProperty<FileInfo> *prop, int index)
{
    FileBrowser *c = static_cast<FileBrowser *>(prop->object);
    return c->m_files.at(index);
}

QQmlListProperty<FileInfo> FileBrowser::dirContent()
{
    return QQmlListProperty<FileInfo>(this, 0, filesCount, fileAt);
}

void FileBrowser::rebuildFilesList()
{
    for (FileInfo *f: m_files) {
        f->deleteLater();
    }
    m_files.clear();
    QStringList files = m_dir.entryList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    for (const QString &s: files) {
        m_files << new FileInfo(m_dir.filePath(s));
    }
}
