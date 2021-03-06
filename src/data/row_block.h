/*!
 *  Copyright (c) 2015 by Contributors
 * \file row_block.h
 * \brief additional data structure to support
 *        RowBlock data structure
 * \author Tianqi Chen
 */
#ifndef DMLC_DATA_ROW_BLOCK_H_
#define DMLC_DATA_ROW_BLOCK_H_
#include <vector>
#include <limits>
#include <dmlc/io.h>
#include <dmlc/logging.h>
#include <dmlc/data.h>

namespace dmlc {
namespace data {
/*!
 * \brief dynamic data structure that holds
 *        a row block of data
 * \tparam IndexType the type of index we are using
 */
template<typename IndexType>
struct RowBlockContainer {
  /*! \brief array[size+1], row pointer to beginning of each rows */
  std::vector<size_t> offset;
  /*! \brief array[size] label of each instance */  
  std::vector<real_t> label;
  /*! \brief feature index */
  std::vector<IndexType> index;
  /*! \brief feature value */
  std::vector<real_t> value;
  /*! \brief maximum value of index */
  IndexType max_index;
  /*! \brief convert to a row block */
  inline RowBlock<IndexType> GetBlock(void) const;
  /*!
   * \brief write the row block to a binary stream
   * \param fo output stream   
   */
  inline void Save(Stream *fo) const;
  /*!
   * \brief load row block from a binary stream
   * \param fi output stream   
   */
  inline void Load(Stream *fi);
  /*! \brief clear the container */
  inline void Clear(void) {
    offset.clear(); offset.push_back(0);
    label.clear(); index.clear(); value.clear();
    max_index = 0;
  }
  /*! \brief size of the data */
  inline size_t Size(void) const {
    return offset.size() - 1;
  }
  /*! 
   * \brief push the row into container
   * \param row the row to push back
   * \tparam I the index type of the row
   */
  template<typename I>
  inline void Push(Row<I> row) {
    label.push_back(row.label);
    for (size_t i = 0; i < row.length; ++i) {
      CHECK(row.index[i] < std::numeric_limits<IndexType>::max())
          << "index exceed numeric bound of current type";
      IndexType findex = static_cast<IndexType>(row.index[i]);
      index.push_back(findex);
      max_index = std::max(max_index, findex);
    }
    if (row.value != NULL) {
      for (size_t i = 0; i < row.length; ++i) {
        value.push_back(row.value[i]);
      }
    }
    offset.push_back(index.size());
   }
  /*! 
   * \brief push the row block into container
   * \param row the row to push back
   * \tparam I the index type of the row
   */
  template<typename I>
  inline void Push(RowBlock<I> batch) {
    size_t size = label.size();
    label.resize(label.size() + batch.size);
    std::memcpy(BeginPtr(label) + size, batch.label,
                batch.size * sizeof(real_t));
    size_t ndata = batch.offset[batch.size];
    index.resize(index.size() + ndata);
    IndexType *ihead = BeginPtr(index) + offset.back();
    for (size_t i = 0; i < ndata; ++i) {
      CHECK(batch.index[i] < std::numeric_limits<IndexType>::max())
          << "index exceed numeric bound of current type";
      IndexType findex = static_cast<IndexType>(batch.index[i]);
      ihead[i] = findex;
      max_index = std::max(max_index, findex);
    }
    if (batch.value != NULL) {
      value.resize(value.size() + ndata);
      std::memcpy(BeginPtr(value) + size, batch.value,
                  ndata * sizeof(real_t));
    }
    size_t shift = offset[size];
    offset.resize(offset.size() + batch.size);
    size_t *ohead = BeginPtr(offset) + size + 1;
    for (size_t i = 0; i < batch.size; ++i) {
      ohead[i] = shift + batch.offset[i + 1];
    }
  }
};

template<typename IndexType>
inline RowBlock<IndexType>
RowBlockContainer<IndexType>::GetBlock(void) const {
  // consistency check
  CHECK(label.size() + 1 == offset.size());
  CHECK(offset.back() == index.size());
  CHECK(offset.back() == value.size() || value.size() == 0);
  RowBlock<IndexType> data;
  data.size = offset.size() - 1;
  data.offset = BeginPtr(offset);
  data.label = BeginPtr(label);
  data.index = BeginPtr(index);
  if (value.size() == 0) {
    data.value = NULL;
  } else {
    data.value = BeginPtr(value);
  }
  return data;
}
template<typename IndexType>
inline void
RowBlockContainer<IndexType>::Save(Stream *fo) const {
  fo->Write(offset);
  fo->Write(label);
  fo->Write(index);
  fo->Write(value);
}
template<typename IndexType>
inline void
RowBlockContainer<IndexType>::Load(Stream *fi) {
  CHECK(fi->Read(&offset)) << "Bad RowBlock format";
  CHECK(fi->Read(&label)) << "Bad RowBlock format"; 
  CHECK(fi->Read(&value)) << "Bad RowBlock format"; 
  CHECK(fi->Read(&index)) << "Bad RowBlock format";
}
}  // namespace data
}  // namespace dmlc
#endif  // DMLC_DATA_ROW_BLOCK_H_
