Hi Mark43 Team!

Heres a quick rundown on my C++ poker file:

- Compiling
	- Using g++, compile the cpp file using the command in your terminal: g++ -std=c++11 \{11-3-17\}-mark43.cpp
	- Output file should be a.out (or whatever filename given in g++ options)
		- This is the file used to run and test, e.g. python run_tests a.out
- Design
	- In this implementation, there are two structs, Card (which has rank and suit) and Hand (which has 3 cards and a hand-type)
		- Cards in a Hand are sorted upon initialization, this makes it easy to compare high cards and find straights
	- Ranks and Hand-Types are represented by enums
		- Since enums implicitly convert to ints while maintaining the initially defined order, it makes comparisons easy and the code easier to read
	- getWinner()
		- First finds the highest poker-hand-type and the highest hand of that type
		- Then adds to the array to be returned any Hands that match both the winning-hand-type and high-card
			- If the winning-hand is a Pair, it will compare the middle cards amond pairs (because given that the cards are sorted, out of 3 cards one of the matching cards will always be in the middle)
