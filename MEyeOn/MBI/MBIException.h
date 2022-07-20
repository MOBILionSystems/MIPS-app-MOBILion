#ifndef MBIEXCEPTION_H
#define MBIEXCEPTION_H
#include <string>

namespace MBI
{
    /*! @class MBI::Exception
    *   @brief Represents a MBIException.
    *   @author Greg Van Aken
    */
    class MBIException {
    public:
        /// @brief Creates an exception with a function name where the failure occurs and an detailed message
        MBIException(const std::string& func_name, const std::string& message) {
            this->func_name = func_name;
            this->detail_message = message;
        }

        /// @brief getDetailMsg Returns the detailed message set at the time the exception is thrown.
        const std::string getDetailMsg() const {
            return detail_message;
        }

        /// @brief getFuncName Returns the function name as a string object set at the time the exception is thrown.
        const std::string getFuncName() const {
            return func_name;
        }
    private:
        std::string detail_message;
        std::string func_name;

    };
}
#endif // MBIEXCEPTION_H
