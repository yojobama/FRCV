%module Native

%{
#include "FRCVLib.h"
%}

// Include standard typemaps and STL support
%include "std_string.i"
%include "std_vector.i"

// Optionally, include other STL containers as needed
// %include "std_map.i"
// %include "std_pair.i"

// Include the main header
%include "FRCVLib.h"