/*
    Copyright 2012 Ulrik Mikaelsson <ulrik.mikaelsson@gmail.com>

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


#include "client.hpp"

#include <boost/assert.hpp>
#include <iostream>

#include "server.hpp"
#include "lib/magneturi.h"

#include <glog/logging.h>

const size_t MAX_ASSETS = 1024;
const size_t MAX_CHUNK = 64*1024;

using namespace std;
namespace fs = boost::filesystem;

using namespace bithorded;

Client::Client( Server& server) :
	bithorde::Client(server.ioService(), server.name()),
	_server(server)
{
}

void Client::onMessage(const bithorde::HandShake& msg)
{
	bithorde::Client::onMessage(msg);
	LOG(INFO) << "Connected: " << msg.name() << endl;
}

void Client::onMessage(const bithorde::BindWrite& msg)
{
	bithorde::Asset::Handle h = msg.handle();
	bithorde::AssetStatus resp;
	resp.set_handle(h);
	if (msg.has_linkpath()) {
		fs::path path(msg.linkpath());
		if (path.is_absolute()) {
			if (_server.linkAsset(path, boost::bind(&Client::onLinkHashDone, shared_from_this(), h, _1))) {
				LOG(INFO) << "Hashing " << path << endl;
				resp.set_status(bithorde::SUCCESS);
			} else {
				LOG(ERROR) << "Upload did not match any allowed assetStore: " << path << endl;
				resp.set_status(bithorde::ERROR);
			}
		}
	} else {
		LOG(ERROR) << "Sorry, upload isn't supported yet" << endl;
		resp.set_status(bithorde::ERROR);
	}
	sendMessage(bithorde::Connection::AssetStatus, resp);
}

void Client::onMessage(const bithorde::BindRead& msg)
{
	auto h = msg.handle();
	if (msg.ids_size() > 0) {
		// Trying to open
		LOG(INFO) << peerName() << ':' << h << " requested: " << MagnetURI(msg) << endl;

		_server.async_findAsset(msg, boost::bind(&Client::onAssetResponse, shared_from_this(), msg, _1));
	} else {
		// Trying to close
		LOG(INFO) << peerName() << ':' << h << " closed" << endl;
		clearAsset(h);
		bithorde::AssetStatus resp;
		resp.set_handle(h);
		resp.set_status(bithorde::NOTFOUND);
		sendMessage(bithorde::Connection::AssetStatus, resp);
	}
}

void Client::onMessage(const bithorde::Read::Request& msg)
{
	bithorde::Read::Response resp;
	resp.set_reqid(msg.reqid());

	Asset::Ptr& asset = getAsset(msg.handle());
	if (asset) {
		uint64_t offset = msg.offset();
		size_t size = msg.size();
		if (size > MAX_CHUNK)
			size = MAX_CHUNK;
		byte buf[MAX_CHUNK];
		auto read = asset->read(offset, size, buf);
		if (read) {
			resp.set_status(bithorde::SUCCESS);
			resp.set_offset(offset);
			resp.set_content(read, size);
		} else {
			resp.set_status(bithorde::NOTFOUND);
		}
	} else {
		resp.set_status(bithorde::INVALID_HANDLE);
	}

	sendMessage(bithorde::Connection::ReadResponse, resp);
}

void Client::onAssetResponse ( const bithorde::BindRead& req, Asset::Ptr a )
{
	bithorde::AssetStatus resp;
	bithorde::Asset::Handle h = req.handle();
	resp.set_handle(h);

	if (a) {
		if (assignAsset(h, a)) {
			LOG(INFO) << peerName() << ':' << h << " found and bound" << endl;
			resp.set_status(bithorde::SUCCESS);
			resp.set_availability(1000);
			resp.set_size(a->size());
			a->getIds(*resp.mutable_ids());
		} else {
			resp.set_status(bithorde::INVALID_HANDLE);
		}
	} else {
		resp.set_status(bithorde::NOTFOUND);
		LOG(INFO) << peerName() << ':' << h << " not found" << endl;
	}

	sendMessage(bithorde::Connection::AssetStatus, resp);
}

void Client::onLinkHashDone(bithorde::Asset::Handle handle, Asset::Ptr a)
{
	bithorde::AssetStatus resp;
	resp.set_handle(handle);
	if (a && a->getIds(*resp.mutable_ids())) {
		resp.set_status(bithorde::SUCCESS);
		resp.set_size(a->size());
	} else {
		resp.set_status(bithorde::ERROR);
	}
	
	sendMessage(bithorde::Connection::AssetStatus, resp);
}

bool Client::assignAsset(bithorde::Asset::Handle handle_, const Asset::Ptr& a)
{
	size_t handle = handle_;
	if (handle >= _assets.size()) {
		if (handle > MAX_ASSETS) {
			LOG(ERROR) << peerName() << ": handle larger than allowed limit (" << handle << " < " << MAX_ASSETS << ")" << endl;
			return false;
		}
		size_t new_size = _assets.size() + (handle - _assets.size() + 1) * 2;
		if (new_size > MAX_ASSETS)
			new_size = MAX_ASSETS;
		_assets.resize(new_size);
	}
	_assets[handle] = a;
	return true;
}

void Client::clearAsset(bithorde::Asset::Handle handle_)
{
	size_t handle = handle_;
	if (handle < _assets.size())
		_assets[handle].reset();
}

Asset::Ptr& Client::getAsset(bithorde::Asset::Handle handle_)
{
	size_t handle = handle_;
	if (handle < _assets.size())
		return _assets.at(handle);
	else
		return ASSET_NONE;
}