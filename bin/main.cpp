// Example of using unrolled_list
#include <iostream>
#include <iterator>

#include <unrolled_list.h>

int main() {
    unrolled_list<int> myList;
    
    myList.push_back(1); // [1]
    myList.push_back(2); // [1, 2]
    myList.push_back(3); // [1, 2, 3]
    
    myList.push_front(0); // Adding an element to the front [0, 1, 2, 3]
    
    myList.insert(std::next(myList.begin()), 10);  // Inserting an element at the next position after begin [0, 10, 1, 2, 3]
    
    std::cout << "After inserts: ";
    for (const auto& elem : myList) {
        std::cout << elem << " ";
    }
    std::cout << std::endl;
    
    myList.erase(std::next(myList.begin(), 2));  // Erasing an element at the position two steps after begin [0, 10, 2, 3]
    
    std::cout << "After erase: ";
    for (const auto& elem : myList) {
        std::cout << elem << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Size of the list: " << myList.size() << std::endl; // 4
    
    return 0;
}