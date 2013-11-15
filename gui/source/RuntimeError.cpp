#include "RuntimeError.hpp"

RuntimeError::RuntimeError(const QString& translatedMessage) : std::runtime_error(translatedMessage.toStdString()),
	translatedMessage(translatedMessage)
{
}

QString RuntimeError::translatedWhat() const
{
	return translatedMessage;
}

RuntimeError::~RuntimeError() throw()
{
}
