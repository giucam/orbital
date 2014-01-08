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

#ifndef CLIBACKEND_H
#define CLIBACKEND_H

#include "hardwareservice.h"

class CliDevice : public Device
{
public:
    CliDevice(const QString &udi);

    bool umount() override;
    bool mount() override;
    bool isMounted() const override;

private:
};

class CliBackend : public HardwareService::Backend
{
public:
    static CliBackend *create(HardwareService *hw);

private:
    CliBackend(HardwareService *hw);
};

#endif
