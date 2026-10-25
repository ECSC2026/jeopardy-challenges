// shapes.mc — classes, inheritance, polymorphism, and the super-call idiom.

class Shape {
    var name;
    function initialize(shapeName) {
        name = shapeName;
    }
    function area() {
        return 0.0;
    }
    function describe() {
        return name + " with area " + area();
    }
}

class Rectangle extends Shape {
    var width;
    var height;
    function initialize(w, h) {
        Shape.initialize("Rectangle");   // call the base constructor
        width = w;
        height = h;
    }
    function area() {
        return width * height;
    }
}

class Square extends Rectangle {
    function initialize(side) {
        Rectangle.initialize(side, side);
        name = "Square";                 // override the inherited field
    }
}

class Circle extends Shape {
    var radius;
    function initialize(r) {
        Shape.initialize("Circle");
        radius = r;
    }
    function area() {
        return 3.14159 * radius * radius;
    }
}

// Polymorphism: one loop, many area() implementations.
var shapes = [new Rectangle(3, 4), new Square(5), new Circle(2)];
for (var i = 0; i < shapes.size(); i += 1) {
    var s = shapes[i];
    println(s.describe());
    println("  is a Shape: " + (s instanceof Shape));
}

// A running total using the polymorphic area().
var totalArea = 0.0;
for (var i = 0; i < shapes.size(); i += 1) {
    totalArea += shapes[i].area();
}
println("Total area: " + totalArea);
