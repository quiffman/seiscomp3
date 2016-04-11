/***************************************************************************
 *   Copyright (C) by GFZ Potsdam                                          *
 *                                                                         *
 *   You can redistribute and/or modify this program under the             *
 *   terms of the SeisComP Public License.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   SeisComP Public License for more details.                             *
 ***************************************************************************/


#include <algorithm>
#include <seiscomp3/core/arrayfactory.h>
#include <seiscomp3/core/typedarray.h>

using namespace Seiscomp;

template<typename TGT, typename SRC>
struct convert {
	TGT operator()(SRC value) {
		return static_cast<TGT>(value);
	}
};

template <typename CONTAINER, typename T>
void convertArray(CONTAINER &c, int size, const T *data) {
	std::transform(data,data+size,std::back_inserter(c),convert<typename CONTAINER::value_type,T>());
}

Array* ArrayFactory::Create(Array::DataType toCreate, Array::DataType caller, int size, const void *data) {
	Array *ar = NULL;

	switch(toCreate) {
	case Array::CHAR: 
		switch (caller) {
		case Array::CHAR:
			ar = new CharArray();
			convertArray(dynamic_cast<CharArray *>(ar)->_data,size,static_cast<const char *>(data));
			break;
		case Array::INT:
			ar = new CharArray();
			convertArray(dynamic_cast<CharArray *>(ar)->_data,size,static_cast<const int *>(data));
			break;
		case Array::FLOAT:
			ar = new CharArray();
			convertArray(dynamic_cast<CharArray *>(ar)->_data,size,static_cast<const float *>(data));
			break;
		case Array::DOUBLE:
			ar = new CharArray();
			convertArray(dynamic_cast<CharArray *>(ar)->_data,size,static_cast<const double *>(data));
			break;
		default:
			break;
		}
		break;
	case Array::INT:
		switch (caller) {
		case Array::CHAR:
			ar = new IntArray();
			convertArray(dynamic_cast<IntArray *>(ar)->_data,size,static_cast<const char *>(data));
			break;
		case Array::INT:
			ar = new IntArray();
			convertArray(dynamic_cast<IntArray *>(ar)->_data,size,static_cast<const int *>(data));
			break;
		case Array::FLOAT:
			ar = new IntArray();
			convertArray(dynamic_cast<IntArray *>(ar)->_data,size,static_cast<const float *>(data));
			break;
		case Array::DOUBLE:
			ar = new IntArray();
			convertArray(dynamic_cast<IntArray *>(ar)->_data,size,static_cast<const double *>(data));
			break;
		default:
			break;
		}
		break;
	case Array::FLOAT:
		switch (caller) {
		case Array::CHAR:
			ar = new FloatArray();
			convertArray(dynamic_cast<FloatArray *>(ar)->_data,size,static_cast<const char *>(data));
			break;
		case Array::INT:
			ar = new FloatArray();
			convertArray(dynamic_cast<FloatArray *>(ar)->_data,size,static_cast<const int *>(data));
			break;
		case Array::FLOAT:
			ar = new FloatArray();
			convertArray(dynamic_cast<FloatArray *>(ar)->_data,size,static_cast<const float *>(data));
			break;
		case Array::DOUBLE:
			ar = new FloatArray();
			convertArray(dynamic_cast<FloatArray *>(ar)->_data,size,static_cast<const double *>(data));
			break;
		default:
			break;
		}
		break;
	case Array::DOUBLE:
		switch (caller) {
		case Array::CHAR:
			ar = new DoubleArray();
			convertArray(dynamic_cast<DoubleArray *>(ar)->_data,size,static_cast<const char *>(data));
			break;
		case Array::INT:
			ar = new DoubleArray();
			convertArray(dynamic_cast<DoubleArray *>(ar)->_data,size,static_cast<const int *>(data));
			break;
		case Array::FLOAT:
			ar = new DoubleArray();
			convertArray(dynamic_cast<DoubleArray *>(ar)->_data,size,static_cast<const float *>(data));
			break;
		case Array::DOUBLE:
			ar = new DoubleArray();
			convertArray(dynamic_cast<DoubleArray *>(ar)->_data,size,static_cast<const double *>(data));
			break;
		default:
			break;
		}
		break;
	default:
		ar = 0;
	}

	return ar;
}

Array* ArrayFactory::Create(Array::DataType toCreate, const Array *source) {
	return Create(toCreate,source->dataType(),source->size(),const_cast<void *>(source->data()));
}

