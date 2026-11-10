/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_RESOURCES_SERVICE_H
#define TRINITYCORE_RESOURCES_SERVICE_H

#include "Service.h"
#include "Client/resource_service.pb.h"

namespace Battlenet::Services
{
    namespace V1
    {
        class Resources : public Service<resources::v1::ResourcesService>
        {
            typedef Service<resources::v1::ResourcesService> ResourcesService;

        public:
            Resources(Session* session);

            uint32 HandleGetTitleIcons(resources::v1::GetTitleIconsRequest const* request, resources::v1::GetTitleIconsResponse* response, std::function<void(ServiceBase*, uint32, ::google::protobuf::Message const*)>& continuation) override;
        };
    }
}

#endif // TRINITYCORE_RESOURCES_SERVICE_H
