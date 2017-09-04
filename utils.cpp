/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2017 Rekols
 *
 * Author:     Rekols <rekols@foxmail.com>
 * Maintainer: Rekols <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "utils.h"
#include <QFile>
#include <QDebug>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>

QString Utils::getQssContent(const QString &path)
{
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QString qss = file.readAll();
    file.close();

    return qss;
}

QString Utils::getUserName()
{
    QString name = qgetenv("USER");

    if (name.isEmpty())
        name = qgetenv("USERNAME");

    return name;
}

QString Utils::getPlatform()
{
    return QString("%1 %2")
           .arg(QSysInfo::kernelType())
           .arg(QSysInfo::currentCpuArchitecture());
}

QString Utils::getDistribution()
{
    return QSysInfo::prettyProductName();
}

QString Utils::getKernel()
{
    return QSysInfo::kernelVersion();
}

void Utils::getCpuInfo(QString &cpuModel, QString &cpuCore)
{
    QFile file("/proc/cpuinfo");
    file.open(QIODevice::ReadOnly);

    QString buffer = file.readAll();
    QStringList model_lines = buffer.split("\n").filter(QRegExp("^model name"));
    QStringList core_lines = buffer.split("\n");

    file.close();

    cpuModel = model_lines.first().split(":").at(1);
    cpuCore = QString::number(core_lines.filter(QRegExp("^processor")).count());
}

unsigned long long Utils::getTotalCpuTime(unsigned long long &workTime)
{
    QFile file("/proc/stat");
    file.open(QIODevice::ReadOnly);
    QString buffer = file.readAll();
    file.close();

    QStringList list = buffer.split("\n").filter(QRegExp("^cpu "));
    QString line = list.first();
    QStringList lines = line.trimmed().split(QRegExp("\\s+"));

    unsigned long long user = lines.at(1).toLong();
    unsigned long long nice = lines.at(2).toLong();
    unsigned long long system = lines.at(3).toLong();
    unsigned long long idle = lines.at(4).toLong();
    unsigned long long iowait = lines.at(5).toLong();
    unsigned long long irq = lines.at(6).toLong();
    unsigned long long softirq = lines.at(7).toLong();
    unsigned long long steal = lines.at(8).toLong();
    //unsigned long long guest = lines.at(9).toLong();
    //unsigned long long guestnice = lines.at(10).toLong();

    workTime = user + nice + system;

    return user + nice + system + idle + iowait + irq + softirq + steal;
}

int Utils::getMemoryPercent(QString &memory)
{
    QFile file("/proc/meminfo");
    file.open(QIODevice::ReadOnly);
    QString info = file.readAll();
    file.close();

    QStringList lines = info.split("\n").filter(QRegExp("^MemTotal|^MemAvailable|^SwapTotal|^SwapFree"));

    QRegExp sep("\\s+");
    quint64 memTotal = lines.at(0).split(sep).at(1).toLong();
    quint64 memAvailable = lines.at(1).split(sep).at(1).toLong();
    quint64 swapTotal = lines.at(2).split(sep).at(1).toLong();
    quint64 swapFree = lines.at(3).split(sep).at(1).toLong();

    memory = QString("%1GB / %2GB").arg(QString::number((memTotal - memAvailable) / 1024.0 / 1024.0, 'r', 1)).arg(QString::number(memTotal / 1024.0 / 1024.0, 'r', 1));

    return int((memTotal - memAvailable) * 100.0 / memTotal);
}

int Utils::getDiskInfo(QString &disk)
{
    QProcess *process = new QProcess;
    process->start("df -Pl");
    process->waitForFinished();

    QTextStream out(process->readAllStandardOutput());
    QStringList result = out.readAll().trimmed().split(QChar('\n'));
    QRegExp sep("\\s+");
    long long size = 0, used = 0, free = 0;

    process->kill();
    process->close();

    for (const QString &line : result.filter(QRegExp("^/")))
    {
        QStringList slist = line.split(sep);
        size = slist.at(1).toLong() << 10;
        used = slist.at(2).toLong() << 10;
        free = slist.at(3).toLong() << 10;
    }

    disk = QString("%1GB / %2GB").arg(QString::number(used / 1024.0 / 1024.0 / 1024.0, 'r', 1)).arg(QString::number(size / 1024.0 / 1024.0 / 1024.0, 'r', 1));

    return used * 100.0 / size;
}

void Utils::getNetworkBandWidth(unsigned long long int &receiveBytes, unsigned long long int &sendBytes)
{
    char buffer[255];
    FILE *fp = fopen("/proc/net/dev", "r");

    // Ignore the first two lines of the file.
    fgets(buffer, 255, fp);
    fgets(buffer, 255, fp);

    receiveBytes = 0;
    sendBytes = 0;

    while (fgets(buffer, 255, fp)) {
        unsigned long long int rBytes, sBytes;
        char *line = strdup(buffer);

        char *dev;
        dev = strtok(line, ":");

        // Filter lo (virtual network device).
        if (QString::fromStdString(dev).trimmed() != "lo") {
            sscanf(buffer + strlen(dev) + 2, "%llu %*d %*d %*d %*d %*d %*d %*d %llu", &rBytes, &sBytes);

            receiveBytes += rBytes;
            sendBytes += sBytes;
        }

        free(line);
    }

    fclose(fp);
}

QString Utils::networkConversion(long bytes)
{
    if (bytes < 1024)
        return QString::number(bytes, 'r', 1) + " B/s";

    if (bytes / 1024 < 1024)
        return QString::number(bytes / 1024, 'r', 1) + "K/s";

    if (bytes / 1024 / 1024 < 1024)
        return QString::number(bytes / 1024 / 1024, 'r', 1) + "M/s";
}

quint64 Utils::getFileSize(const QString &path)
{
    quint64 totalSize = 0;

    QFileInfo info(path);

    if (info.exists()) {
        if (info.isFile()) {
            totalSize += info.size();
        }else if (info.isDir()) {
            QDir dir(path);

            for (const QFileInfo &i : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs)) {
                totalSize += getFileSize(i.absoluteFilePath());
            }
        }
    }

    return totalSize;
}

QString Utils::formatBytes(const quint64 &bytes)
{
#define formatUnit(v, u, t) QString().sprintf("%.1f %s", \
    ((double) v / (double) u), t)

    if (bytes == 1L) // bytes
        return QString("%1 byte").arg(bytes);
    else if (bytes < KIBI) // bytes
      return QString("%1 bytes").arg(bytes);
    else if (bytes < MEBI) // KiB
      return formatUnit(bytes, KIBI, "KB");
    else if (bytes < GIBI) // MiB
      return formatUnit(bytes, MEBI, "MB");
    else if (bytes < TEBI) // GiB
      return formatUnit(bytes, GIBI, "GB");
    else if (bytes < PEBI) // TiB
      return formatUnit(bytes, TEBI, "TB");
    else if (bytes < EXBI) // PiB
      return formatUnit(bytes, PEBI, "PB");
    else                   // EiB
      return formatUnit(bytes, EXBI, "EB");

#undef formatUnit
}

QFileInfoList Utils::getDpkgPackages()
{
    QDir reports("/var/cache/apt/archives");

    return reports.entryInfoList(QDir::Files);
}

QFileInfoList Utils::getCrashReports()
{
    QDir reports("/var/crash");

    return reports.entryInfoList(QDir::Files);
}

QFileInfoList Utils::getAppLogs()
{
    QDir logs("/var/log");

    return logs.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
}

QFileInfoList Utils::getAppCaches()
{
    QString homePath = getHomePath();
    QDir caches(homePath + "/.cache");

    return caches.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
}

QString Utils::getHomePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

QString exec(const QString &cmd, QStringList args)
{
    QProcess *process = new QProcess;

    if (args.isEmpty())
        process->start(cmd);
    else
        process->start(cmd, args);

    process->waitForFinished();
    QString out = process->readAllStandardOutput();
    QString error = process->errorString();

    process->kill();
    process->close();

    if (process->error() != QProcess::UnknownError)
        throw error;

    return out.trimmed();
}

QString Utils::sudoExec(const QString &cmd, QStringList args)
{
    args.push_front(cmd);

    QString result("");

    try {
        result = exec("pkexec", args);
    } catch (QString &ex) {
        qCritical() << ex;
    }

    return result;
}
