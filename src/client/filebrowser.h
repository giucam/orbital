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
 * Nome-Programma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
 */

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
