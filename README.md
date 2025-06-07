# UnrolledLinkedList
STL-compliant UnrolledLinkedList container

## Description: 

STL-compatible container for [UnrolledLinkedList](https://en.wikipedia.org/wiki/Unrolled_linked_list)

The container is implemented as a template parameterized by the type of stored objects, the maximum number of elements per node, and an allocator. It also meets the following requirements for STL-compatible containers:

  - [Container](https://en.cppreference.com/w/cpp/named_req/Container)
  - [SequenceContainer](https://en.cppreference.com/w/cpp/named_req/SequenceContainer)
  - [ReversibleContainer](https://en.cppreference.com/w/cpp/named_req/ReversibleContainer)
  - [AllocatorAwareContainer](https://en.cppreference.com/w/cpp/named_req/AllocatorAwareContainer)
  - [Has BidirectionalIterator](https://en.cppreference.com/w/cpp/named_req/BidirectionalIterator)

## Asymptotic complexity for some methods


| Method      |  Algorithmic complexity         | Exception safety    |  
| ----------  | ------------------------------  | ------------------- |  
| insert      |  O(1) for 1 element, O(M) for M |  strong             |  
| erase       |  O(1) for 1 element, O(M) for M |  noexcept           |  
| clear       |  O(N)                           |  noexcept           |  
| push_back   |  O(1)                           |  strong             |  
| pop_back    |  O(1)                           |  noexcept           |  
| push_front  |  O(1)                           |  strong             |  
| pop_front   |  O(1)                           |  noexcept           |  


## Tests

  All functionality is covered by tests using the Google Test framework