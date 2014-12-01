/*
  Copyright (C) 2011 by the Computer Poker Research Group, University of Alberta
*/

/*
 * NOTE
 *
 * change random_time = 1000 to random_time = 10000 in actFlop
 */
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
#include <time.h>
#include "game.h"
#include "rng.h"
#include "net.h"

#include "evalHandTables"

#define MAX_CARDS (MAX_SUITS * MAX_RANKS)

#define v (state.viewingPlayer)
#define oppv (1 - state.viewingPlayer)

#define getholeCardRank(i) (rankOfCard(state.state.holeCards[v][(i)]))
#define getholeCardSuit(i) (suitOfCard(state.state.holeCards[v][(i)]))

#define getOppHoleCardRank(i) (rankOfCard(state.state.holeCards[oppv][(i)]))
#define getOppHoleCardSuit(i) (suitOfCard(state.state.holeCards[oppv][(i)]))

#define holeCard(i) (state.state.holeCards[v][(i)])

#define getBoardCardRank(i) (rankOfCard(state.state.boardCards[(i)]))
#define getBoardCardSuit(i) (suitOfCard(state.state.boardCards[(i)]))

#define boardCard(i) (state.state.boardCards[(i)])
#define getMoney(i) (state.state.spent[v ^ (i)])
#define getMyMoney() getMoney(0)
#define getOppMoney() getMoney(1)


double win[13][13] = {
{49.410000, 29.290000, 30.140000, 31.210000, 30.910000, 31.680000, 33.990000, 36.340000, 38.940000, 41.650000, 44.560000, 47.730000, 52.240000},
{33.120000, 52.840000, 32.080000, 33.160000, 32.900000, 33.670000, 34.640000, 37.230000, 39.840000, 42.540000, 45.460000, 48.610000, 53.080000},
{33.950000, 35.740000, 56.260000, 35.080000, 34.820000, 35.610000, 36.600000, 37.890000, 40.750000, 43.430000, 46.360000, 49.510000, 53.950000},
{34.930000, 36.760000, 38.540000, 59.650000, 36.830000, 37.630000, 38.630000, 39.940000, 41.540000, 44.470000, 47.390000, 50.540000, 54.940000},
{34.680000, 36.510000, 38.300000, 40.160000, 62.580000, 39.390000, 40.440000, 41.780000, 43.420000, 45.160000, 48.290000, 51.440000, 54.910000},
{35.400000, 37.250000, 39.060000, 40.930000, 42.600000, 65.720000, 42.580000, 43.870000, 45.510000, 47.280000, 49.320000, 52.680000, 56.360000},
{37.580000, 38.160000, 39.980000, 41.870000, 43.580000, 45.570000, 68.720000, 45.850000, 47.490000, 49.250000, 51.340000, 53.680000, 57.540000},
{39.780000, 40.610000, 41.220000, 43.140000, 44.850000, 46.780000, 48.630000, 71.690000, 49.470000, 51.160000, 53.250000, 55.640000, 58.600000},
{42.240000, 43.080000, 43.910000, 44.650000, 46.390000, 48.350000, 50.210000, 52.050000, 74.660000, 53.340000, 55.310000, 57.720000, 60.700000},
{44.790000, 45.620000, 46.450000, 47.410000, 48.030000, 50.010000, 51.870000, 53.650000, 55.670000, 77.160000, 56.250000, 58.640000, 61.630000},
{47.560000, 48.410000, 49.230000, 50.180000, 51.020000, 51.960000, 53.850000, 55.620000, 57.560000, 58.430000, 79.630000, 59.580000, 62.570000},
{50.580000, 51.390000, 52.200000, 53.150000, 53.980000, 55.130000, 56.080000, 57.890000, 59.820000, 60.690000, 61.580000, 82.100000, 63.490000},
{54.840000, 55.600000, 56.380000, 57.310000, 57.260000, 58.610000, 59.720000, 60.680000, 62.660000, 63.550000, 64.410000, 65.280000, 84.970000},
};
double tie[13][13] = {
{1.910000, 6.910000, 6.950000, 6.970000, 6.800000, 6.560000, 6.310000, 6.020000, 5.740000, 5.500000, 5.300000, 5.110000, 4.900000},
{6.540000, 1.730000, 6.950000, 6.980000, 6.810000, 6.580000, 6.300000, 6.040000, 5.760000, 5.520000, 5.320000, 5.130000, 4.910000},
{6.570000, 6.580000, 1.550000, 6.940000, 6.810000, 6.590000, 6.300000, 6.030000, 5.780000, 5.550000, 5.330000, 5.150000, 4.920000},
{6.610000, 6.610000, 6.590000, 1.380000, 6.650000, 6.470000, 6.190000, 5.910000, 5.670000, 5.460000, 5.250000, 5.060000, 4.840000},
{6.430000, 6.450000, 6.460000, 6.320000, 1.180000, 6.130000, 5.910000, 5.620000, 5.370000, 5.170000, 4.980000, 4.800000, 4.580000},
{6.210000, 6.240000, 6.250000, 6.150000, 5.850000, 1.040000, 5.540000, 5.300000, 5.030000, 4.820000, 4.660000, 4.490000, 4.300000},
{5.970000, 5.970000, 6.000000, 5.900000, 5.640000, 5.290000, 0.910000, 4.910000, 4.680000, 4.460000, 4.280000, 4.140000, 3.960000},
{5.700000, 5.730000, 5.730000, 5.630000, 5.360000, 5.070000, 4.710000, 0.800000, 4.320000, 4.140000, 3.950000, 3.780000, 3.610000},
{5.440000, 5.460000, 5.490000, 5.400000, 5.120000, 4.820000, 4.490000, 4.150000, 0.720000, 3.780000, 3.650000, 3.470000, 3.290000},
{5.230000, 5.250000, 5.270000, 5.200000, 4.940000, 4.610000, 4.280000, 3.990000, 3.640000, 0.650000, 3.430000, 3.250000, 3.060000},
{5.020000, 5.050000, 5.070000, 5.000000, 4.760000, 4.450000, 4.100000, 3.790000, 3.510000, 3.310000, 0.610000, 3.070000, 2.870000},
{4.850000, 4.880000, 4.890000, 4.820000, 4.580000, 4.300000, 3.960000, 3.640000, 3.340000, 3.140000, 2.970000, 0.580000, 2.750000},
{4.630000, 4.670000, 4.690000, 4.610000, 4.360000, 4.110000, 3.790000, 3.490000, 3.180000, 2.950000, 2.780000, 2.660000, 0.570000},
};


double winProbability(const int card1, const int card2)
{
    int rank_1 = rankOfCard(card1);
    int rank_2 = rankOfCard(card2);
    int rank1 = rank_1 < rank_2 ? rank_1 : rank_2;
    int rank2 = rank_1 < rank_2 ? rank_2 : rank_1;
    if (suitOfCard(card1) == suitOfCard(card2))
        return ((win[rank2][rank1] + tie[rank2][rank1]/2) / 100);
    else
        return ((win[rank1][rank2] + tie[rank1][rank2]/2) / 100);
}

void calcPrep(const double place, const int total_money, double probs[3])
{
    double threshold[4] = {0.41, 0.50, 0.65, 0.8};
    //double threshold[4] = {0.00, 0.50, 0.50, 1.0};
    double hig = 10.0;
    if (place <= threshold[0])
    {
        probs[a_fold] = 1.00;
        probs[a_call] = 0.001;
        probs[a_raise] = 0.0;
    }
    else if (place <= threshold[1])
    {
        probs[a_fold] = -hig / (threshold[1] - threshold[0])*(place - threshold[1]);
        probs[a_call] = 1.0;
        probs[a_raise] = 0.0;
    }
    else if (place <= threshold[2])
    {
        probs[a_fold] = 0.0;
        probs[a_call] = 1.0;
        probs[a_raise] = 0.0;
    }
    else if (place <= threshold[3])
    {
        probs[a_fold] = 0.0;
        probs[a_call] = 1.0;
        probs[a_raise] = hig / (threshold[3] - threshold[2])*(place - threshold[2]);
    }
    else
    {
        probs[a_fold] = 0.0;
        probs[a_call] = 0.001;
        probs[a_raise] = 1.00;
    }
}

void calcFlop(const double place, const int total_money, double probs[3])
{
    double threshold[4] = {0.00, 0.4999, 0.50, 1.0};
    double hig = 10.0;
    if (place <= threshold[0])
    {
        probs[a_fold] = 1.00;
        probs[a_call] = 0.001;
        probs[a_raise] = 0.0;
    }
    else if (place <= threshold[1])
    {
        probs[a_fold] = -hig / (threshold[1] - threshold[0])*(place - threshold[1]);
        probs[a_call] = 1.0;
        probs[a_raise] = 0.0;
    }
    else if (place <= threshold[2])
    {
        probs[a_fold] = 0.0;
        probs[a_call] = 1.0;
        probs[a_raise] = 0.0;
    }
    else if (place <= threshold[3])
    {
        probs[a_fold] = 0.0;
        probs[a_call] = 1.0;
        probs[a_raise] = hig / (threshold[3] - threshold[2])*(place - threshold[2]);
    }
    else
    {
        probs[a_fold] = 0.0;
        probs[a_call] = 0.001;
        probs[a_raise] = 1.00;
    }
/*
    if (place >= (3.0 / 4.0))
    {
        probs[a_fold] = 0.0;
        probs[a_call] = 0.25;
        probs[a_raise] = 0.75;
    } else if (place >= (1.0 / 2.0)) {
        probs[a_fold] = 0.0;
        probs[a_call] = 0.5;
        probs[a_raise] = 0.5;
    } else {*/
        /*
         * NOTE!
         * last workable value:
         * probs[a_fold] = 0.75;
         * probs[a_call] = 0.25;
         */
         /*
        probs[a_fold] = 1-place;
        probs[a_call] = place;
        probs[a_raise] = 0.0;
    }*/
}

void calcRiver(const double place, const int total_money, double probs[3])
{
    /*
     * the same as
     * calcFlop(place, total_money, probs);
     */
    // fprintf(stderr, "place = %f\n", place);
    calcFlop(place, total_money, probs);
    /*
    if (place >= (3.0 / 4.0))
    {
        probs[a_fold] = 0.0;
        probs[a_call] = 0.25;
        probs[a_raise] = 0.75;
    } else if (place >= (1.0 / 2.0)) {
        probs[a_fold] = 0.0;
        probs[a_call] = 0.5;
        probs[a_raise] = 0.5;
    } else {
        probs[a_fold] = 0.50;
        probs[a_call] = 0.50;
        probs[a_raise] = 0.0;
    }*/

}

void calcTurn(const double place, const int total_money, double probs[3])
{
    /*
     * the same as
     * calcFlop(place, total_money, probs);
     */
    // fprintf(stderr, "place = %f\n", place);
    calcFlop(place, total_money, probs);
    /*
    if (place >= (3.0 / 4.0))
    {
        probs[a_fold] = 0.0;
        probs[a_call] = 0.25;
        probs[a_raise] = 0.75;
    } else if (place >= (1.0 / 2.0)) {
        probs[a_fold] = 0.0;
        probs[a_call] = 0.5;
        probs[a_raise] = 0.5;
    } else {
        probs[a_fold] = 0.50;
        probs[a_call] = 0.50;
        probs[a_raise] = 0.0;
    }
*/
}



static void xorCardToCardset( Cardset *c, int suit, int rank )
{
  c->cards ^= (uint64_t)1 << ( ( suit << 4 ) + rank );
}

void copyCardset(Cardset *dst, const Cardset *src)
{
    int i;
    for(i = 0; i < 4; i++)
        dst->bySuit[i] = src->bySuit[i];
    dst->cards = src->cards;
}
void addCardToCardset__( Cardset *c, int card)
{
    addCardToCardset(c, suitOfCard(card), rankOfCard(card));
}

void xorCardToCardset__(Cardset *c, int card)
{
    xorCardToCardset(c, suitOfCard(card), rankOfCard(card));
}

int inCardset(const Cardset *c, const int card)
{
    return (c->cards & (uint64_t)1 << (( suitOfCard(card) << 4) + rankOfCard(card))) > 0;
}

uint8_t totalNumBoardCards[MAX_ROUNDS];

Game *game;
MatchState state;

/*
 * calc totalNumBoardCards
 * Q: what is numBoardCards?
 */
void preCalculate()
{
    memset(totalNumBoardCards, 0, sizeof(totalNumBoardCards));
    int i, j;
    for (i = 0; i < MAX_ROUNDS; i++)
    {
        for (j = 0; j <= i; j++)
            totalNumBoardCards[i] = totalNumBoardCards[i] + game->numBoardCards[j];
    }
    srand(time(NULL));
}

void addBoardCard(Cardset *set)
{
    int i;
    for (i = 0; i < totalNumBoardCards[state.state.round]; i++)
        addCardToCardset( set, getBoardCardSuit(i), getBoardCardRank(i) );
}

void actPref(double probs[NUM_ACTION_TYPES])
{
    double prob = winProbability(holeCard(0), holeCard(1));
    calcPrep(prob, 0,probs);
}

void cheetRounds(int *count)
{
    int myNum;
    int i;

    Cardset evalSet = emptyCardset();

    addBoardCard(&evalSet);
    for (i = 0; i < MAX_HOLE_CARDS; i++)
        addCardToCardset(&evalSet, getholeCardSuit(i), getholeCardRank(i));
    myNum = rankCardset(evalSet);

    evalSet = emptyCardset();
    addBoardCard(&evalSet);
    for (i = 0; i < MAX_HOLE_CARDS; i++)
        addCardToCardset(&evalSet, getOppHoleCardSuit(i), getOppHoleCardSuit(i));
    if (rankCardset(evalSet) < myNum)
    {
        //maybe plus the case that we fold?
        (*count)++;
    }
}

void actPref__(double probs[NUM_ACTION_TYPES])
{
    double prob[4][3] = {
        {0.4, 0.6, 0.0},
        {0.0, 0.4, 0.0},
        {0.0, 0.4, 0.0},
        {0.0, 0.2, 0.0}
    };


    uint8_t rank0, rank1, suit0, suit1;
    int index;
    uint8_t temp;
    rank0 = getholeCardRank(0);
    rank1 = getholeCardRank(1);
    suit0 = getholeCardSuit(0);
    suit1 = getholeCardSuit(1);


    if (suit0 != suit1) {
        if (rank0 != rank1) { /* high */
            temp = rank0 + rank1;
            if (temp <= 16)
                if (temp <= 8)
                    index = 0;
                else
                    index = 1;
            else
                if (temp <= 24)
                    index = 2;
                else
                    index = 3;
        } else { /* pair */
            temp = rank0;
            if (temp <= 8)
                if (temp <= 2)
                    index = 0;
                else
                    index = 1;
            else
                if (temp <= 10)
                    index = 2;
                else
                    index = 3;
        }
    } else { /* flush */
        temp = rank0 + rank1;
        if (temp <= 14)
            if (temp <= 5)
                index = 0;
            else
                index = 1;
        else
            if (temp <= 20)
                index = 2;
            else
                index = 3;
    }

    probs[a_fold] = prob[index][a_fold];
    probs[a_call] = prob[index][a_call];
    probs[a_raise] = prob[index][a_raise];

    // the following is the origin one
    /* probs[a_call] = 1.0; */
    /* probs[a_fold] = 0.0; */
    /* probs[a_raise] = 0.0; */
    //   Cardset evalset = emptyCardset();
    //   if
}

void actFlop(double probs[NUM_ACTION_TYPES])
{

    Cardset mySet;
    Cardset evalSet;
    int card1, card2;
    int cCard1, cCard2;
    int numPosi = (MAX_CARDS - 5) * (MAX_CARDS - 4) / 2;

    int myNum, tempNum;
    int count = 0;
    const int random_time = 10000;
    int i = random_time;

    while(i > 0){
        mySet = emptyCardset();
        addBoardCard(&mySet);
        copyCardset(&evalSet, &mySet);
        addCardToCardset__(&mySet, holeCard(0));
        addCardToCardset__(&mySet, holeCard(1));
        while(inCardset(&mySet, (cCard1 = rand() % MAX_CARDS)))
            ;
        addCardToCardset__(&mySet, cCard1);
        addCardToCardset__(&evalSet, cCard1);
        while(inCardset(&mySet, (cCard2 = rand() % MAX_CARDS)))
            ;
        addCardToCardset__(&mySet, cCard2);
        addCardToCardset__(&evalSet, cCard2);
        while(inCardset(&mySet, (card1 = rand() % MAX_CARDS)))
            ;
        addCardToCardset__(&evalSet, card1);
        while(inCardset(&mySet, (card2 = rand() % MAX_CARDS)))
            ;
        addCardToCardset__(&evalSet, card2);
        myNum = rankCardset(mySet);
        tempNum = rankCardset(evalSet);
        if (myNum > tempNum)
            count += 2;
        else if (myNum == tempNum)
            count++;

        i--;
    }

    calcFlop((double)count/(2*random_time), 0, probs);
}

void actFlop__(double probs[NUM_ACTION_TYPES])
{
    Cardset mySet = emptyCardset();
    Cardset evalSet = emptyCardset();
    int card1, card2;
    int cCard1, cCard2;
    int numPosi = (MAX_CARDS - 5) * (MAX_CARDS - 6) / 2;

    int myNum, tempNum;
    int count = 0;
    int i;
    addBoardCard(&mySet);
    addCardToCardset(&mySet, getholeCardSuit(0), getholeCardRank(0));
    addCardToCardset(&mySet, getholeCardSuit(1), getholeCardRank(1));
    myNum = rankCardset(mySet);


    for (card1 = 0; card1 < MAX_CARDS; card1++)
        for (card2 = card1+1; card2 < MAX_CARDS; card2++)
        {
            evalSet = emptyCardset();
            addBoardCard(&evalSet);
            if (inCardset(&evalSet, card1) || inCardset(&evalSet, card2))
                continue;

            addCardToCardset(&evalSet, suitOfCard(card1), rankOfCard(card1));
            addCardToCardset(&evalSet, suitOfCard(card2), rankOfCard(card2));

            if (rankCardset(evalSet) <= myNum)
                count++;
        }

    calcFlop((double)count/(double)numPosi, getMyMoney(), probs);
}


void actTurn(double probs[NUM_ACTION_TYPES])
{
    // actFlop(probs);
    Cardset mySet = emptyCardset();
    Cardset evalSet;
    int card1, card2, cCard;
    int myNum;
    int tempNum;
    int numPosi = (MAX_CARDS - 6) * (MAX_CARDS - 7) * (MAX_CARDS - 8) / 2;
    /*
     * count = (#smaller hands)*2
     */
    unsigned long count = 0;
    unsigned long total = 0;
    int i;

    addBoardCard(&mySet);
    copyCardset(&evalSet, &mySet);
    addCardToCardset(&mySet, getholeCardSuit(0), getholeCardRank(0));
    addCardToCardset(&mySet, getholeCardSuit(1), getholeCardRank(1));

    for (cCard = 0; cCard < MAX_CARDS; cCard++){
        if (inCardset(&mySet, cCard))
            continue;

        addCardToCardset(&mySet, suitOfCard(cCard), rankOfCard(cCard));
        addCardToCardset(&evalSet, suitOfCard(cCard), rankOfCard(cCard));
        myNum = rankCardset(mySet);

        for (card1 = 0; card1 < MAX_CARDS; card1++){
            if (inCardset(&evalSet, card1))
                continue;

            addCardToCardset(&evalSet, suitOfCard(card1), rankOfCard(card1));
            for (card2 = card1+1; card2 < MAX_CARDS; card2++)
            {
                if (inCardset(&evalSet, card2))
                    continue;

                addCardToCardset(&evalSet, suitOfCard(card2), rankOfCard(card2));
                total += 2;
                if ((tempNum = rankCardset(evalSet)) < myNum)
                    count += 2;
                else if (tempNum == myNum)
                    count++;
                xorCardToCardset(&evalSet, suitOfCard(card2), rankOfCard(card2));
            }
            xorCardToCardset(&evalSet, suitOfCard(card1), rankOfCard(card1));
        }
        xorCardToCardset(&mySet, suitOfCard(cCard), rankOfCard(cCard));
        xorCardToCardset(&evalSet, suitOfCard(cCard), rankOfCard(cCard));
    }
    // fprintf(stderr, "count = %lu, total = %lu\n", count, total);
    calcTurn((double)count/total, getMyMoney(), probs);
}

void actTurn__(double probs[NUM_ACTION_TYPES])
{
    actFlop__(probs);
}


void actRiver(double probs[NUM_ACTION_TYPES])
{
    // actFlop(probs);
    Cardset mySet = emptyCardset();
    Cardset evalSet;
    int card1, card2;
    int myNum;
    int tempNum;
    int numPosi = (MAX_CARDS - 7) * (MAX_CARDS - 8) / 2;
    // int totalMoney = getMoney(); /* to be get from Mr. Tri */
    /*
     * count = (#smaller hands)*2
     */
    int count = 0;
    int total = 0;
    int i;

    addBoardCard(&mySet);
    copyCardset(&evalSet, &mySet);
    addCardToCardset(&mySet, getholeCardSuit(0), getholeCardRank(0));
    addCardToCardset(&mySet, getholeCardSuit(1), getholeCardRank(1));
    myNum = rankCardset(mySet);

    for (card1 = 0; card1 < MAX_CARDS; card1++){
        if (inCardset(&evalSet, card1))
            continue;

        addCardToCardset(&evalSet, suitOfCard(card1), rankOfCard(card1));
        for (card2 = card1+1; card2 < MAX_CARDS; card2++)
        {
            if (inCardset(&evalSet, card2))
                continue;

            addCardToCardset(&evalSet, suitOfCard(card2), rankOfCard(card2));
            total += 2;
            if ((tempNum = rankCardset(evalSet)) < myNum)
                count += 2;
            else if (tempNum == myNum)
                count++;
            xorCardToCardset(&evalSet, suitOfCard(card2), rankOfCard(card2));
        }
        xorCardToCardset(&evalSet, suitOfCard(card1), rankOfCard(card1));
    }
    calcRiver((double)count/total, getMyMoney(), probs);
}

void actRiver__(double probs[NUM_ACTION_TYPES])
{
    actFlop__(probs);
}


int main( int argc, char **argv )
{
    int sock, len, r, a;
    int32_t min, max;
    uint16_t port;
    double p;
    Action action;
    FILE *file, *toServer, *fromServer;
    struct timeval tv;
    double probs[ NUM_ACTION_TYPES ];
    double actionProbs[ NUM_ACTION_TYPES ];
    rng_state_t rng;
    char line[ MAX_LINE_LEN ];

    int statenum, i, j, round, cumu, view;
    int cheetTimes = 0;
    int roundThree = 0;

    /* we make some assumptions about the actions - check them here */
    assert( NUM_ACTION_TYPES == 3 );

    if( argc < 4 ) {

        fprintf( stderr, "usage: player game server port\n" );
        exit( EXIT_FAILURE );
    }

    /* Define the probabilities of actions for the player */
    probs[ a_fold ] = 0.06;
    probs[ a_call ] = ( 1.0 - probs[ a_fold ] ) * 0.5;
    probs[ a_raise ] = ( 1.0 - probs[ a_fold ] ) * 0.5;

    /* Initialize the player's random number state using time */
    gettimeofday( &tv, NULL );
    init_genrand( &rng, tv.tv_usec );

    /* get the game */
    file = fopen( argv[ 1 ], "r" );
    if( file == NULL ) {

        fprintf( stderr, "ERROR: could not open game %s\n", argv[ 1 ] );
        exit( EXIT_FAILURE );
    }
    game = readGame( file );
    if( game == NULL ) {

        fprintf( stderr, "ERROR: could not read game %s\n", argv[ 1 ] );
        exit( EXIT_FAILURE );
    }
    fclose( file );

    /* connect to the dealer */
    if( sscanf( argv[ 3 ], "%"SCNu16, &port ) < 1 ) {

        fprintf( stderr, "ERROR: invalid port %s\n", argv[ 3 ] );
        exit( EXIT_FAILURE );
    }
    sock = connectTo( argv[ 2 ], port );
    if( sock < 0 ) {

        exit( EXIT_FAILURE );
    }
    toServer = fdopen( sock, "w" );
    fromServer = fdopen( sock, "r" );
    if( toServer == NULL || fromServer == NULL ) {

        fprintf( stderr, "ERROR: could not get socket streams\n" );
        exit( EXIT_FAILURE );
    }

    /* send version string to dealer */
    if( fprintf( toServer, "VERSION:%"PRIu32".%"PRIu32".%"PRIu32"\n",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION ) != 14 ) {

        fprintf( stderr, "ERROR: could not get send version to server\n" );
        exit( EXIT_FAILURE );
    }
    fflush( toServer );

    /* precalculate some tools */
    preCalculate();

    /* play the game! */
    while( fgets( line, MAX_LINE_LEN, fromServer ) ) {

        /* ignore comments */
        if( line[ 0 ] == '#' || line[ 0 ] == ';' ) {
            continue;
        }

        len = readMatchState( line, game, &state );
        if( len < 0 ) {

            fprintf( stderr, "ERROR: could not read state %s", line );
            exit( EXIT_FAILURE );
        }

        if( stateFinished( &state.state ) ) {
            /* ignore the game over message */

            continue;
        }

        round = state.state.round;
        statenum = state.state.handId;
        view = state.viewingPlayer;

        if( currentPlayer( game, &state.state ) != state.viewingPlayer ) {
            /* we're not acting */
            if (round == 3)
            {
                cheetRounds(&cheetTimes);
                roundThree++;
            }
            continue;
        }


        /* Round 0: preflop, 1: flop, 2: turn, 3: river */
        /* statenum is the state */

        switch (round)
        {
        case 0:
            actPref(probs);
            break;
        case 1:
            actFlop(probs);
            break;
        case 2:
            actTurn(probs);
            break;
        case 3:
            actRiver(probs);
            break;
        }
        /* add a colon (guaranteed to fit because we read a new-line in fgets) */
        line[ len ] = ':';
        ++len;

        /* build the set of valid actions */
        p = 0;
        for( a = 0; a < NUM_ACTION_TYPES; ++a ) {
            actionProbs[ a ] = 0.0;
        }

        /* consider fold */
        action.type = a_fold;
        action.size = 0;
        if(isValidAction(game, &state.state, 0, &action)) {
            actionProbs[ a_fold ] = probs[ a_fold ];
            p += probs[ a_fold ];
        }

        /* consider call */
        action.type = a_call;
        action.size = 0;
        actionProbs[ a_call ] = probs[ a_call ];
        p += probs[ a_call ];

        /* consider raise */
        if( raiseIsValid( game, &state.state, &min, &max ) ) {

            actionProbs[ a_raise ] = probs[ a_raise ];
            p += probs[ a_raise ];
        }

        /* normalise the probabilities  */
        assert( p > 0.0 );
        for( a = 0; a < NUM_ACTION_TYPES; ++a ) {

            actionProbs[ a ] /= p;
        }

        /* choose one of the valid actions at random */
        p = genrand_real2( &rng );
        for( a = 0; a < NUM_ACTION_TYPES - 1; ++a ) {

            if( p <= actionProbs[ a ] ) {

                break;
            }
            p -= actionProbs[ a ];
        }
        action.type = (enum ActionType)a;
        if( a == a_raise ) {

            action.size = min + genrand_int32( &rng ) % ( max - min + 1 );
        }

        /* do the action! */
        assert( isValidAction( game, &state.state, 0, &action ) );
        r = printAction( game, &action, MAX_LINE_LEN - len - 2,
                         &line[ len ] );
        if( r < 0 ) {

            fprintf( stderr, "ERROR: line too long after printing action\n" );
            exit( EXIT_FAILURE );
        }
        len += r;
        line[ len ] = '\r';
        ++len;
        line[ len ] = '\n';
        ++len;

        if( fwrite( line, 1, len, toServer ) != len ) {

            fprintf( stderr, "ERROR: could not get send response to server\n" );
            exit( EXIT_FAILURE );
        }
        fflush( toServer );
    }

    return EXIT_SUCCESS;
}
