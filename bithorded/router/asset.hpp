/*
    Copyright 2012 <copyright holder> <email>

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef BITHORDED_ROUTER_ASSET_H
#define BITHORDED_ROUTER_ASSET_H

#include <map>
#include <memory>

#include "../server/asset.hpp"
#include "../server/client.hpp"
#include "../../lib/asset.h"
#include "../../lib/client.h"

namespace bithorded {
namespace router {
class Router;

struct PendingRead {
	uint64_t offset;
	size_t size;
	IAsset::ReadCallback cb;
};

class UpstreamAsset : public bithorde::ReadAsset, public boost::enable_shared_from_this<UpstreamAsset> {
public:
	typedef boost::shared_ptr<UpstreamAsset> Ptr;
	explicit UpstreamAsset(const ClientPointer& client, const BitHordeIds& requestIds);
	virtual void handleMessage(const bithorde::Read::Response& resp);
};

class ForwardedAsset : public bithorded::IAsset, public boost::noncopyable, public boost::enable_shared_from_this<ForwardedAsset>
{
	Router& _router;
	BitHordeIds _ids;
	int64_t _size;
	std::map<std::string, UpstreamAsset::Ptr> _upstream;
	std::list<PendingRead> _pendingReads;
public:
	typedef boost::shared_ptr<ForwardedAsset> Ptr;
	typedef boost::weak_ptr<ForwardedAsset> WeakPtr;

	ForwardedAsset(Router& router, const BitHordeIds& ids) :
		_router(router),
		_ids(ids),
		_size(-1),
		_upstream(),
		_pendingReads()
	{}

	bool hasUpstream(const std::string peername);
	void bindUpstreams(const std::map< std::string, bithorded::Client::Ptr >& friends, uint64_t uuid, int timeout);

	virtual size_t can_read(uint64_t offset, size_t size);
	virtual bool getIds(BitHordeIds& ids) const;
	virtual void async_read(uint64_t offset, size_t& size, uint32_t timeout, bithorded::IAsset::ReadCallback cb);
	virtual uint64_t size();

	virtual void inspect(management::InfoList& target) const;
	void inspect_upstreams(management::InfoList& target) const;
private:
	void onUpstreamStatus(const std::string& peername, const bithorde::AssetStatus& status);
	void updateStatus();
	void onData(uint64_t offset, const std::string& data, int tag);
	void dropUpstream(const std::string& peername);
};

}
}

#endif // BITHORDED_ROUTER_ASSET_H
