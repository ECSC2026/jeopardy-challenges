var some_array = new [50];

for (var i = 0; i < some_array.size(); i += 1) {
    some_array[i] = [1,2,3,4];
}

// Free some array
some_array[30] = null;