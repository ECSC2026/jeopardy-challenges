// VulnVM runtime prelude — the Toybox.Lang exception hierarchy, as ordinary
// classes. Compiled once at build time (by vulnvmc) into a serialized blob that
// the interpreter loads in vm_new before running a user program. Keep in sync
// with the exception types the VM raises.
class Exception {
    var mMessage;
    function initialize() {}
    function getErrorMessage() { return mMessage; }
    function printStackTrace() {}
}
class InvalidValueException extends Exception {}
class InvalidOptionsException extends Exception {}
class UnexpectedTypeException extends Exception {}
class ValueOutOfBoundsException extends Exception {}
class OperationNotAllowedException extends Exception {}
class SymbolNotAllowedException extends Exception {}
class SerializationException extends Exception {}
class StorageFullException extends Exception {}
