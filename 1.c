#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <getopt.h>
#include "game.h"
#include "rng.h"
#include "net.h"

#define SUIT_NUM 4
#define CARD_NUM 15
#define DEFAULT 255

typedef unsigned char u8;

/*
 * 2-10 :2-10
 * 11 : J
 * 12 : Q
 * 13 : K
 * 14 : A
 */

typedef struct
{
	u8 suit;
	u8 number;
} card;

card card_seq[7];
//<rank> := '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' | 'T' | 'J' | 'Q' | 'K' | 'A'
//<suit> := 's' | 'h' | 'd' | 'c'
uint8_t raw2rank(const char raw_suit)
{
	switch (raw_suit)
	{
        case 'T':
                return 10;
                break;
        case 'J':
                return 11;
                break;
        case 'Q':
                return 12;
                break;
        case 'K':
                return 13;
                break;
        case 'A':
                return 14;
                break;
        default: return (uint8_t)(raw_suit - '0');
	}
	return 0;
}
uint8_t raw2suit(const char raw_rank)
{
	switch (raw_rank)
	{
		case 's': return 0;
		case 'h': return 1;
		case 'd': return 2;
		case 'c': return 3;
	}
	return 0;
}
void raw2card(const char raw_card[2], card* this_card)
{
	this_card->number = raw2rank(raw_card[0]);
	this_card->suit = raw2suit(raw_card[1]);
}


u8 suit_1[SUIT_NUM];
u8 num_1[CARD_NUM];
u8 suit_2[SUIT_NUM];
u8 num_2[CARD_NUM];

void printnum(const u8 num[CARD_NUM])
{
        int i;
        for(i = 0; i < CARD_NUM-1; i++)
                printf("%d ", num[i]);
        printf("%d\n", num[i]);
}

        
int check_flush(const u8 suit[SUIT_NUM], u8 *flush_suit)
{
        int i;
        for (i = 0; i < SUIT_NUM; i++)
                if (suit[i] >= 5){ /* only one i can satisfy suit_1[i] >= 5 */
                        if (flush_suit)
                                *flush_suit = i;
                        return 1;
                }
        return 0;
}

int check_straight(const u8 num[CARD_NUM], u8 *start_num)
{
        u8 count = 0, start = 0;
        int i;
        *start_num = start;
        for (i = CARD_NUM - 1; i >= 0; --i)
        {
                if (num[i])
                        count++;
                else
                        if (count < 5)
                                count = 0;
                
                if (count >= 5 && !start){
                        if (start_num)
                                *start_num = i + 4;
                        return 1;
                }
        }
        return 0;
}

/* check for four of a kind, return 1 is so */
int check_four(const u8 num[CARD_NUM], u8 *_four, u8 *_one)
{
        int i;
        u8 four = DEFAULT, one = DEFAULT;
        for (i = CARD_NUM - 1; i >= 0; --i)
        {
                if (four == DEFAULT && num[i] >= 4){
                        four = i;
                } else {
                        if (one == DEFAULT && num[i] >= 1)
                                one = i;
                }
        }
        if (four != DEFAULT && one != DEFAULT){
                if (_four)
                        *_four = four;
                if (_one)
                        *_one = one;
                return 1;
        } else
                return 0;

}


/*
 * check for fullhouse, return 1 if fullhouse
 * else return 0.
 *
 * NOTE:
 * the caller responsible to call check_four
 * before caling check_fullhouse
 */
int check_fullhouse(const u8 num[CARD_NUM], u8 *_three, u8 *_two)
{
        int i;
        u8 three = DEFAULT, two = DEFAULT;
        for (i = CARD_NUM - 1; i >= 0; --i)
        {
                if (three == DEFAULT && num[i] >= 3){
                        three = i;
                } else {
                        if (two == DEFAULT && num[i] >= 2)
                                two = i;
                }
        }
        if (three != DEFAULT && two != DEFAULT){
                if (_three)
                        *_three = three;
                if (_two)
                        *_two = two;
                return 1;
        } else
                return 0;
}

/*
 * check for three of a kind
 *
 * the caller ensures calling check_fullhouse, etc,
 * before calling it
 */
int check_three(const u8 num[CARD_NUM], u8 *_three, u8 *_one, u8 *_another)
{
        int i;
        u8 three = DEFAULT, one = DEFAULT, another = DEFAULT;
        for (i = CARD_NUM - 1; i >= 0; --i)
        {
                if (three == DEFAULT && num[i] >= 3){
                        three = i;
                } else {
                        if (num[i] >= 1){
                                if (one == DEFAULT)
                                        one = i;
                                else
                                        if (another == DEFAULT)
                                                another = i;
                        }
                        
                }
        }
        if (three != DEFAULT && one != DEFAULT && another != DEFAULT){
                if (_three)
                        *_three = three;
                if (_one)
                        *_one = one;
                if (_another)
                        *_another = another;
                return 1;
        } else
                return 0;
        
}
/*
 * check for a pair and two pairs
 *
 * the caller ensures calling check_three, etc,
 * before calling it
 * return 2 if two pairs
 * return 1 if a pair
 * return 0 if high card
 */
int check_two(const u8 num[CARD_NUM], u8 *_pair1, u8 *_pair2, u8 *_c1, u8 *_c2, u8 *_c3)
{
        int i;
        u8 pair1, pair2, c1, c2, c3;
        pair1 = pair2 = c1 = c2 = c3 = DEFAULT;
        for (i = CARD_NUM - 1; i >= 0; --i)
        {
                if (num[i] >= 2){
                        if (pair1 == DEFAULT)
                                pair1 = i;
                        else if (pair2 == DEFAULT)
                                pair2 = i;
                } else {
                        if (num[i] >= 1){
                                if (c1 == DEFAULT)
                                        c1 = i;
                                else if (c2 == DEFAULT)
                                        c2 = i;
                                else if (c3 == DEFAULT)
                                        c3 = i;
                        }
                        
                }
        }
        if (_pair1)
                *_pair1 = pair1;
        if (_pair2)
                *_pair2 = pair2;
        if (_c1)
                *_c1 = c1;
        if (_c2)
                *_c2 = c2;
        if (_c3)
                *_c3 = c3;
        
        return ((pair1 != DEFAULT) + (pair2 != DEFAULT));
}


/*
 * calc the max value of an array
 */
u8 calc_max(u8 num[CARD_NUM])
{
        u8 ret = 0;
        int i;
        for (i = 0; i < CARD_NUM; ++i)
                if (num[i] > ret)
                        ret = num[i];
        return ret;
}

void calc_flush_(const card player[2], const card all[5], const int flush, u8 num[CARD_NUM])
{
        int i;
        for(i = 0; i < CARD_NUM; ++i)
                num[i] = 0;
        for(i = 0; i < 5; ++i) /* 5 common cards */
                if (all[i].suit == flush)
                        num[all[i].number]++;
        for(i = 0; i < 2; ++i) /* 2 private cards */
                if (player[i].suit == flush)
                        num[player[i].number]++;
}

/*
 * Calc the number of different cards, i.e. how many
 * 1, 2, etc are in the set of a certain suit
 * flush_suit can be 0, 1, 2, 3.
 */

void calc_flush(const card player1[2], const int p1_flush, const card player2[2], const int p2_flush, const card all[5], u8 num_1[CARD_NUM], u8 num_2[CARD_NUM])
{
        calc_flush_(player1, all, p1_flush, num_1);
        calc_flush_(player2, all, p2_flush, num_2);
}

void calc_(const card player[2], const card all[5], u8 num[CARD_NUM], u8 suit[SUIT_NUM])
{
        int i;
        for(i = 0; i < CARD_NUM; ++i)
                num[i] = 0;
        for(i = 0; i < SUIT_NUM; ++i)
                suit[i] = 0;
        for(i = 0; i < 5; ++i){ /* 5 common cards */
                num[all[i].number]++;
                suit[all[i].suit]++;
        }
        for(i = 0; i < 2; ++i){ /* 2 private cards */
                num[player[i].number]++;
                suit[player[i].suit]++;
        }
}


void calc(const card player1[2], const card player2[2], const card all[5])
{
        calc_(player1, all, num_1, suit_1);
        calc_(player2, all, num_2, suit_2);
}

int *type(const card player[2], const card all[5])
{
        int *retVal;
        u8 num[CARD_NUM], suit[SUIT_NUM];
        u8 num_[CARD_NUM];
        int is_flush, is_straight, is_fullhouse, is_four;
        u8 flush, start, three, two, four, one;
        retVal = calloc(sizeof(int), 6);
        calc_(player, all, num, suit);
        // printnum(num);
        is_flush = check_flush(suit, &flush);
        if (is_flush){
                retVal[0] = 6;
                calc_flush_(player, all, flush, num_);
                is_straight = check_straight(num_, &start);
                if (is_straight){
                        retVal[1] = start;
                        retVal[0] = 9 + (start == 14);/* A = 14 */
                        return retVal;
                }
        }

        is_four = check_four(num, &four, &one);
        if (is_four){
                retVal[0] = 8;
                retVal[1] = four;
                retVal[2] = one;
                return retVal;
        }
        
        is_fullhouse = check_fullhouse(num, &three, &two);
        if (is_fullhouse){
                retVal[0] = 7;
                retVal[1] = three;
                retVal[2] = two;
                return retVal;
        }

        if (is_flush){
                retVal[0] = 6;
                int count = 1;
                int i = CARD_NUM;
                while(count <= 5){
                        while(num_[i] == 0)
                                i--;
                        retVal[count] = i;
                        num_[i]--;
                }
                return retVal;
        }

        is_straight = check_straight(num, &start);
        if (is_straight){
                retVal[0] = 5;
                retVal[1] = start;
                return retVal;
        }

        u8 another;
        int is_three = check_three(num, &three, &one, &another);
        if (is_three){
                retVal[0] = 4;
                retVal[1] = three;
                retVal[2] = one;
                retVal[3] = another;
                return retVal;
        }

        u8 pair1, pair2, c1, c2, c3;
        int is_two = check_two(num, &pair1, &pair2, &c1, &c2, &c3);
        retVal[0] = is_two;
        if (is_two == 2){
                retVal[1] = pair1;
                retVal[2] = pair2;
                retVal[3] = c1;
                return retVal;
        }
        if (is_two == 1){
                retVal[1] = pair1;
                retVal[2] = c1;
                retVal[3] = c2;
                retVal[4] = c3;
                return retVal;
        }

        int count = 1;
        int i = CARD_NUM - 1;
        while (count <= 5){
                while(num[i] == 0)
                        i--;
                retVal[count] = i;
                count++;
                i--;
        }
        return retVal;
        
}


int cmp_type(int *type1, int *type2)
{
        int i = 0;
        while (i < 6){
                if (type1[i] != type2[i])
                        return type1[i] - type2[i];
                else
                        i++;
        }
        return 0;
}
void readline_card(FILE* file, int num, card player[2], card all[5])
{
	int i;
	char raw_card[2];
	for (i = 0; i < 2; i++)
	{
		fscanf(file, "%c%c", &raw_card[0], &raw_card[1]);
                //printf("%c%c", raw_card[0], raw_card[1]);
                
		raw2card(raw_card, &(player[i]));
	}
	fscanf(file, " ");
	for (i = 0; i < 5; i++)
	{
		fscanf(file, "%c%c", &raw_card[0], &raw_card[1]);
		raw2card(raw_card, &(all[i]));
	}
	fscanf(file, "\n");
}

int main(int argc, char** argv)
{
	FILE *file;
	//number of sets
        card player1[2]; // , player2[2];
        card all[5];
	int num;
	int* ptr;
	int i, j;

	file = fopen(argv[1], "r");
	fscanf(file, "%d\n", &num);
        
	for (i = 0; i < num; i++)
	{
		readline_card(file, num, player1, all);
                for(j = 0; j < 5-1; j++)
                        printf("%d ", all[j].number);
                printf("%d\n", all[j].number);
                ptr = type(player1, all);

		for (j = 0; j < 5; j++)
			printf("%d ", ptr[j]);
		printf("%d\n", ptr[5]);
	}
	fclose(file);
        return 0;
}

/* 
int main()
{
        card player1[2], player2[2];
        card all[5];
        int i;
        int *type1, *type2;
        assign_card(player1[0]);
        assign_card(player1[1]);
        assign_card(player2[0]);
        assign_card(player2[1]);
        for (i = 0; i < 5; i++)
                assign_card(all[i]);
        type1 = type(player1, all);
        type2 = type(player2, all);
        print(type1);
        print(type2);
        printf("%d\n", cmp_type(type1, type2));
        
}

*/
        
