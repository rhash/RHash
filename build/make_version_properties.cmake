# make_version_properties.cmake
#
#VERSION_PROP_FILE = file that contains version property
#VERSION = version as 1.x.x
#
#This script ensures that the property file contains the appropriate version number

File(WRITE ${VERSION_PROP_FILE} "version=${VERSION}")
