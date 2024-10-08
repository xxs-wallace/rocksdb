//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <table/block_based/block_cache.h>
#include <table/block_based/cachable_entry.h>
#include <trace_replay/block_cache_tracer.h>

#include <cinttypes>
#include <memory>

#include "db/blob/blob_read_request.h"
#include "file/random_access_file_reader.h"
#include "rocksdb/compression_type.h"
#include "rocksdb/rocksdb_namespace.h"
#include "util/autovector.h"

namespace ROCKSDB_NAMESPACE {

class Status;
struct ImmutableOptions;
struct FileOptions;
class HistogramImpl;
struct ReadOptions;
class Slice;
class FilePrefetchBuffer;
class BlobContents;
class Statistics;

struct BlobVersion {

};

class BlobFileReader {
  std::unique_ptr<RandomAccessFileReader> file_reader_;
  uint64_t file_num;
  uint64_t file_size_;
  uint64_t smallest_block_id_;
  uint64_t largest_block_id_;
  CompressionType compression_type_;
  SystemClock* clock_;
  Statistics* statistics_;
};

class BlockReader {
 public:
  static Status Create(const ImmutableOptions& immutable_options,
                       const ReadOptions& read_options,
                       const FileOptions& file_options,
                       uint32_t column_family_id,
                       std::vector<uint64_t> blob_file_number,
                       const std::shared_ptr<IOTracer>& io_tracer,
                       std::unique_ptr<BlockReader>* reader);

  BlockReader(const BlockReader&) = delete;
  BlockReader& operator=(const BlockReader&) = delete;

  ~BlockReader();
  Status ReadBlockAndLoadToCache(
    const ReadOptions& ro, const BlockIndexHandle& handle, const UncompressionDict& uncompression_dict,
    bool for_compaction, CachableEntry<Block_kData>* out_parsed_block,
    BlockCacheLookupContext* lookup_context,
    BlockContents* contents, bool async_read,
    bool use_block_cache_for_lookup);

  Status GetBlock(const ReadOptions& read_options, const Slice& user_key,
                 uint64_t offset, uint64_t value_size,
                 CompressionType compression_type,
                 FilePrefetchBuffer* prefetch_buffer,
                 MemoryAllocator* allocator,
                 std::unique_ptr<BlobContents>* result,
                 uint64_t* bytes_read) const;

 private:
  BlockReader(std::unique_ptr<RandomAccessFileReader>&& file_reader,
                 uint64_t file_size, CompressionType compression_type,
                 SystemClock* clock, Statistics* statistics);

  Status OpenFile(const ImmutableOptions& immutable_options,
                         const FileOptions& file_opts,
                         HistogramImpl* blob_file_read_hist,
                         uint64_t blob_file_number,
                         const std::shared_ptr<IOTracer>& io_tracer,
                         uint64_t* file_size,
                         std::unique_ptr<RandomAccessFileReader>* file_reader);


  using Buffer = std::unique_ptr<char[]>;

  static Status ReadFromFile(const RandomAccessFileReader* file_reader,
                             const ReadOptions& read_options,
                             uint64_t read_offset, size_t read_size,
                             Statistics* statistics, Slice* slice, Buffer* buf,
                             AlignedBuf* aligned_buf);

  static Status VerifyBlob(const Slice& record_slice, const Slice& user_key,
                           uint64_t value_size);

  static Status UncompressBlobIfNeeded(const Slice& value_slice,
                                       CompressionType compression_type,
                                       MemoryAllocator* allocator,
                                       SystemClock* clock,
                                       Statistics* statistics,
                                       std::unique_ptr<BlobContents>* result);

  std::vector<std::unique_ptr<BlobFileReader>> file_readers_;
};

}  // namespace ROCKSDB_NAMESPACE
