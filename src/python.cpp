#include <boost/python.hpp>
#include "seqdb.h"

using namespace boost::python;

BOOST_PYTHON_MODULE(seqdb)
{
	class_<SeqDB>("SeqDB", init<const char*, char, size_t, size_t>())
		.def("size", &SeqDB::size)
		.def("write", &SeqDB::write);
	def("open", &SeqDB::open);
	def("create", &SeqDB::create);
}

