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

#ifndef SOLIDBACKEND_H
#define SOLIDBACKEND_H

#include <solid/device.h>

#include "hardwareservice.h"

class SolidDevice : public Device
{
public:
    SolidDevice(const QString &udi);

    bool umount() override;
    bool mount() override;
    bool isMounted() const override;

private:
    Solid::Device m_device;
};

class SolidBackend : public HardwareService::Backend
{
public:
    static SolidBackend *create(HardwareService *hw);

private:
    SolidBackend(HardwareService *hw);
};

#endif
