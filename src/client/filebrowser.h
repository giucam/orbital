
#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <QObject>
#include <QDir>

class FileInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
public:
    FileInfo(const QString &file);

    QString name() const;

    Q_INVOKABLE bool isDir() const;
    Q_INVOKABLE QString path() const;

signals:
    void nameChanged();

private:
    QFileInfo m_info;
};

class FileBrowser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters)
    Q_PROPERTY(QQmlListProperty<FileInfo> dirContent READ dirContent NOTIFY pathChanged)
public:
    FileBrowser(QObject *p = nullptr);

    void setPath(const QString &path);
    void setNameFilters(const QStringList &filters);

    QString path() const;
    QStringList nameFilters() const;
    QQmlListProperty<FileInfo> dirContent();

public slots:
    void cdUp();
    void cd(const QString &dir);

signals:
    void pathChanged();

private:
    void rebuildFilesList();
    static int filesCount(QQmlListProperty<FileInfo> *prop);
    static FileInfo *fileAt(QQmlListProperty<FileInfo> *prop, int index);

    QDir m_dir;
    QList<FileInfo *> m_files;
};

#endif
