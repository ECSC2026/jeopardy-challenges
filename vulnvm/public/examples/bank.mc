// bank.mc — a small program tying many features together: classes with private
// state, custom exceptions, a static counter, a module, and collections.

class InsufficientFundsException extends Exception {
    function initialize(needed, available) {
        mMessage = "need " + needed + " but only " + available + " available";
    }
}

class Account {
    static var nextId = 1000;

    private var _id;
    private var _owner;
    private var _balance;
    private var _history;

    function initialize(owner, opening) {
        _id = Account.nextId;
        Account.nextId += 1;
        _owner = owner;
        _balance = opening;
        _history = [];
        record("open", opening);
    }

    function record(kind, amount) {
        _history.add({:kind => kind, :amount => amount});
    }

    function getId()      { return _id; }
    function getOwner()   { return _owner; }
    function getBalance() { return _balance; }

    function deposit(amount) {
        _balance += amount;
        record("deposit", amount);
    }

    function withdraw(amount) {
        if (amount > _balance) {
            throw new InsufficientFundsException(amount, _balance);
        }
        _balance -= amount;
        record("withdraw", amount);
    }

    function transactionCount() {
        return _history.size();
    }
}

module Bank {
    function transfer(from, to, amount) {
        from.withdraw(amount);   // may throw before `to` is touched
        to.deposit(amount);
    }
}

var alice = new Account("Alice", 100);
var bob   = new Account("Bob", 50);

println("Opened account #" + alice.getId() + " for " + alice.getOwner());
println("Opened account #" + bob.getId() + " for " + bob.getOwner());

alice.deposit(25);
println(alice.getOwner() + " balance: " + alice.getBalance());

Bank.transfer(alice, bob, 40);
println("After transfer of 40:");
println("  " + alice.getOwner() + ": " + alice.getBalance());
println("  " + bob.getOwner() + ": " + bob.getBalance());

// This transfer overdraws and is rejected.
try {
    Bank.transfer(bob, alice, 1000);
} catch (e instanceof InsufficientFundsException) {
    println("Transfer rejected: " + e.getErrorMessage());
}

println("Balances unchanged:");
println("  " + alice.getOwner() + ": " + alice.getBalance());
println("  " + bob.getOwner() + ": " + bob.getBalance());
println(alice.getOwner() + " made " + alice.transactionCount() + " transactions");
