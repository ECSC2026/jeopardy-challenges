// exceptions.mc — try/catch/finally, custom exceptions, and error recovery.

// A custom exception type.
class NegativeInputException extends Exception {
    function initialize(msg) {
        mMessage = msg;
    }
}

function factorial(n) {
    if (n < 0) {
        throw new NegativeInputException("factorial of a negative number: " + n);
    }
    var result = 1;
    for (var i = 2; i <= n; i += 1) {
        result *= i;
    }
    return result;
}

// Catch a custom exception by type.
try {
    println("5! = " + factorial(5));
    println("(-3)! = " + factorial(-3));
} catch (e instanceof NegativeInputException) {
    println("Caught: " + e.getErrorMessage());
}

// Runtime errors are catchable too.
function safeDivide(a, b) {
    try {
        return a / b;
    } catch (e) {
        println("Division failed (" + e.getErrorMessage() + "), returning 0");
        return 0;
    }
}
println("10 / 2 = " + safeDivide(10, 2));
println("10 / 0 = " + safeDivide(10, 0));

// finally always runs.
function withCleanup(shouldThrow) {
    try {
        if (shouldThrow) { throw new Exception(); }
        println("  work completed");
    } catch (e) {
        println("  handled an error");
    } finally {
        println("  cleanup ran");
    }
}
println("Normal path:");
withCleanup(false);
println("Error path:");
withCleanup(true);

// Out-of-bounds access, caught.
var data = [10, 20, 30];
try {
    var x = data[99];
} catch (e) {
    println("Index error caught: " + e.getErrorMessage());
}
