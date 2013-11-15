#ifndef RuntimeError_hpp
#define RuntimeError_hpp

#include <stdexcept>
#include <QString>

/**
  * @class  RuntimeError
  * @brief  a QString-aware exception class
  */
class RuntimeError : public std::runtime_error {
public:
	explicit RuntimeError(const QString& translatedMessage);
	QString translatedWhat() const;
	~RuntimeError() throw();

private:
	QString translatedMessage;
};

#endif
