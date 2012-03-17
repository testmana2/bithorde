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


#ifndef RANDOMACCESSFILE_H
#define RANDOMACCESSFILE_H

#include <boost/filesystem/path.hpp>

#include "lib/types.h"

class RandomAccessFile {
	int _fd;
public:
	const static size_t WINDOW_SIZE = 65536;

	RandomAccessFile(const boost::filesystem3::path& path);
	~RandomAccessFile();

	/**
	 * The number of bytes in the open file
	 */
	uint64_t size();

	/**
	 * The number of blocks of /blockSize/ required to hold all file content
	 */
	uint blocks(size_t blockSize);

	/**
	 * Reads up to /size/ bytes from file and returns a pointer to the data.
	 *
	 * @arg buf - a buffer allocated with at least /size/ capacity
	 * @arg size - will be updated with actual read amount
	 * @returns pointer to data, may or may not be pointer to /buf/
	 */
	byte* read(uint64_t offset, size_t& size, byte *buf);

	/**
	 * Writes up to /size/ bytes to file beginning at /offset/.
	 */
	ssize_t write(uint64_t offset, void* src, size_t size);
};

#endif // RANDOMACCESSFILE_H
