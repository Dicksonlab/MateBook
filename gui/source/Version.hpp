#ifndef Version_hpp
#define Version_hpp

/**
  * @class  Version
  * @brief  compatibility information
  */
class Version {
public:
	static const unsigned int current;	// the version of projects saved with this program
	static const unsigned int earliestResettableProject;	// not supported, but can be reset to "Recording Finished"
	static const unsigned int earliestConvertibleProject;	// can be converted to the current version (if the file structure / file format didn't change, conversion is a no-op)
};

#endif
