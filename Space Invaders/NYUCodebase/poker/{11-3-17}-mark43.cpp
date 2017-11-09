#include <iostream>
#include <array>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

// Card object contains a rank and suite
struct Card
{
	enum Rank { Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King, Ace };
	Card(char charRank, char suit) : suit(suit)
	{
		Rank finalRank;
		if (charRank >= '2' && charRank <= '9') finalRank = static_cast<Rank>(charRank - '2');
		else
		{
			switch (charRank) {
			case 'T': finalRank = Ten; break;
			case 'J': finalRank = Jack; break;
			case 'Q': finalRank = Queen; break;
			case 'K': finalRank = King; break;
			case 'A': finalRank = Ace; break;
			}
		}
		rank = finalRank;
	}
	// Operators only compare values
	bool operator<(const Card& rhs) const
	{
		return rank < rhs.rank;
	}
	bool operator>(const Card& rhs) const
	{
		return rank > rhs.rank;
	}
	bool operator==(const Card& rhs) const
	{
		return rank == rhs.rank;
	}
	Rank rank;
	char suit;
};

// Hand object contains 3 cards and a poker-hand type
struct Hand
{
	enum Type { High_Card, Pair, Flush, Straight, Three_Pair, Straight_Flush };
	Hand(const Card c1, const Card c2, const Card c3) : cards{ c1, c2, c3 }
	{
		sort(cards.begin(), cards.end());
		type = findType(); // Can't call in initialization list else the cards will not be sorted
	}
	bool operator>(const Hand& rhs) const
	{
		if (cards[2] > rhs.cards[2]) return true;
		else if (cards[2] == rhs.cards[2] && cards[1] > rhs.cards[1]) return true;
		else if (cards[2] == rhs.cards[2] && cards[1] == cards[1] && cards[0] > cards[0]) return true;
		else return false;
	}
	bool operator==(const Hand& rhs) const
	{
		return cards == rhs.cards;
	}
	array<Card, 3> cards; // 3-card poker, 3-cards in a hand
	Type type;
private:
	Type findType()
	{
		// Method for finding straights work assuming the cards are sorted
		const bool straight = ((cards[2].rank == cards[1].rank + 1) && (cards[2].rank == cards[0].rank + 2));
		const bool flush = (cards[0].suit == cards[1].suit) && (cards[0].suit == cards[2].suit);
		if (straight && flush) return Straight_Flush;
		else if (cards[0].rank == cards[1].rank && cards[0].rank == cards[2].rank) return Three_Pair;
		else if (straight) return Straight;
		else if (flush) return Flush;
		else if (cards[0].rank == cards[1].rank || cards[0].rank == cards[2].rank || cards[1].rank == cards[2].rank)
			return Pair;
		else return High_Card;
	}
};

vector<int> getWinner(vector<Hand> hands)
{
	vector<int> winners;
	// Assume first hand is the winning hand
	Hand::Type winType = hands[0].type;
	int highHand = 0;
	for (size_t i = 1; i < hands.size(); ++i)
	{
		if(hands[i].type > winType)
		{
			winType = hands[i].type;
			highHand = i;
		}
		else if(hands[i].type == winType && hands[i] > hands[highHand])
		{
			highHand = i;
		}
	}
	// Special comparison case for pairs
	if (winType == Hand::Type::Pair)
	{
		for (size_t i = 0; i < hands.size(); ++i)
		{
			// Since all cards are sorted and hands only contain 3 cards, if there is a pair, one of the cards in the matching pair has to be in the middle index
			if (hands[i].type == winType && hands[i].cards[1] == hands[highHand].cards[1]) winners.push_back(i);
		}
	}
	else
	{
		for (size_t i = 0; i < hands.size(); ++i) {
			if (hands[i].type == winType && hands[i] == hands[highHand]) winners.push_back(i);
		}
	}
	return winners;
}

int main()
{
	int playerCount = 0;
	cin >> playerCount;
	vector<Hand> hands;
	int id;
	string s1, s2, s3;
	// Input handling
	for (int i = 0; i < playerCount; ++i)
	{
		cin >> id >> s1 >> s2 >> s3;

		// Optimized for this very specific input
		Card c1 = { s1[0], s1[1] };
		Card c2 = { s2[0], s2[1] };
		Card c3 = { s3[0], s3[1] };

		hands.emplace_back(c1, c2, c3);
	}
	// Print winners
	for (int winner : getWinner(hands))
	{
		cout << winner << " ";
	}
}

