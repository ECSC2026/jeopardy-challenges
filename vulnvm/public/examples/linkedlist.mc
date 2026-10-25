// linkedlist.mc — a small data structure: a singly linked list and a stack,
// plus a module of helpers.

class Node {
    var value;
    var next;
    function initialize(v) {
        value = v;
        next = null;
    }
}

class LinkedList {
    var head;
    var count;
    function initialize() {
        head = null;
        count = 0;
    }
    function prepend(v) {
        var node = new Node(v);
        node.next = head;
        head = node;
        count += 1;
    }
    function size() {
        return count;
    }
    function toArray() {
        var result = [];
        var cur = head;
        while (cur != null) {
            result.add(cur.value);
            cur = cur.next;
        }
        return result;
    }
    function sum() {
        var total = 0;
        var cur = head;
        while (cur != null) {
            total += cur.value;
            cur = cur.next;
        }
        return total;
    }
}

class Stack {
    var items;
    function initialize() {
        items = [];
    }
    function push(v) {
        items.add(v);
    }
    function pop() {
        var top = items[items.size() - 1];
        items = items.slice(0, items.size() - 1);
        return top;
    }
    function isEmpty() {
        return items.size() == 0;
    }
}

module ListUtil {
    function makeRange(n) {
        var list = new LinkedList();
        for (var i = n; i >= 1; i -= 1) {
            list.prepend(i);
        }
        return list;
    }
}

var list = ListUtil.makeRange(5);
println("List:  " + list.toArray());
println("Size:  " + list.size());
println("Sum:   " + list.sum());

var stack = new Stack();
stack.push("a");
stack.push("b");
stack.push("c");
print("Popping: ");
while (!stack.isEmpty()) {
    print(stack.pop() + " ");
}
println("");
