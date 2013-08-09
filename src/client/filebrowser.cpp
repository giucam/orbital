/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
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

void FileBrowser::cdHome()
{
    setPath(QDir::homePath());
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
