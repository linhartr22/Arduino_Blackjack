/* BlackJack.
 *
 * Author: Roger D. Linhart  linhartr@gmail.com  8/13/2013
 *
 * Version 1.0
 *
 * Rules: Dealer hits stands on all 17's. Splitting and Doubling Down are not implemented.
 *
 * Notes: I've experienced issues with the special character for the "10" custom character. I don't know
 * if this is a bug in the program or a hardware glitch with my LDC Display.
 *
 * If you found this fun or interesting you are encouraged to make a small donation to my PayPal account
 * at https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=GTBD7T7BXPGQY
 * I have many more Arduino projects in mind and appreciate your support.
 *
 * No commercial use without prior authorization. Thank you. 
 *
 * LCD Display Module Interface.
 * LCD RS pin to digital pin 8.
 * LCD Enable pin to digital pin 9.
 * LCD D4 pin to digital pin 4.
 * LCD D5 pin to digital pin 5.
 * LCD D6 pin to digital pin 6.
 * LCD D7 pin to digital pin 7.
 * LCD Keypad Module Interface.
 * Key Codes.
 * None   - 0.
 * Select - 1.
 * Left   - 2.
 * Up     - 3.
 * Down   - 4.
 * Right  - 5.
 */

// Initialize the LiquidCrystal.
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
const byte lcdRowMax = 2;
const byte lcdColMax = 16;

// Create suit symbols character maps.
byte heart[8] = {
  B01010,
  B11111,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000,
};
byte diamond[8] = {
  B00100,
  B01110,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000,
};
byte spade[8] = {
  B00100,
  B01110,
  B11111,
  B11111,
  B00100,
  B01110,
  B00000,
  B00000,
};
byte club[8] = {
  B01110,
  B01110,
  B11111,
  B11111,
  B00100,
  B01110,
  B00000,
  B00000,
};
byte back1[8] = {
  B11111,
  B10110,
  B10101,
  B10110,
  B10101,
  B10110,
  B10000,
  B11111,
};
byte back2[8] = {
  B11111,
  B00001,
  B01111,
  B00101,
  B00101,
  B10101,
  B01001,
  B11111,
};
byte ten[8] = {
  B01000,
  B11000,
  B01000,
  B01010,
  B11101,
  B00101,
  B00101,
  B00010,
};

// Initialize LCD Keypad.
#include <DFR_Key.h>
DFR_Key keypad;

// Key constants.
const byte keySelect = 1;
const byte keyLeft = 2;
const byte keyUp = 3;
const byte keyDown = 4;
const byte keyRight = 5;

// Init Deck.
const byte cardIndexMax = 52;
const byte rankIndex = 0;
const byte suitIndex = 1;
byte deck[cardIndexMax][2];
byte cardIndex;

// Init Hand.
const byte handIndexMax = 6;
const byte dealerIndex = 0;
const byte playerIndex = 1;
byte hand[2][handIndexMax][2];
byte playerHandIndex;
byte dealerHandIndex;
boolean dealerHoleCard = true;  // Hole Card display state.
byte handTotal[2][2];           // Need two totals per player for Ace duality.

// Init Bank.
// Bank values are Dollars and Cents since a 1.5 Blackjack payout can result in fractional dollars.
const long bankInit = 10000;
long bank = bankInit;
const long bankMax = 100000;
const long betMin = 500;
const long betMax = 5000;
const long betInc = 500;

// Setup.
void setup() {
  // Init serial for debugging.
  Serial.begin(9600);

  // Init card characters.
  lcd.createChar(0, heart);
  lcd.createChar(1, diamond);
  lcd.createChar(2, spade);
  lcd.createChar(3, club);
  lcd.createChar(4, back1);
  lcd.createChar(5, back2);
  lcd.createChar(7, ten);

  // Set up the LCD's number of columns and rows.
  lcd.begin(lcdColMax, lcdRowMax);

  // Create deck.
  cardIndex = 0;
  for (byte suit = 0; suit <= 3 ; suit++) {
    for (byte rank = 1; rank <= 13; rank++) {
      deck[cardIndex][rankIndex] = rank;
      deck[cardIndex++][suitIndex] = suit;
    }
  }

  // Seed random number generator.
  unsigned long seed=0, count=32;
  while (--count) {
    seed = (seed<<1) | (analogRead(0)&1);
  }
  randomSeed(seed);
}

// Main loop.
void loop() {
  // Display splash.
  displaySplash();

  // Got any money left?
  while (bank > 0 && bank < bankMax) {
    // Yes, Get bet.
    int bet = getBet();

    // Start a new round.
    dealerHandIndex = 0;
    playerHandIndex = 0;
    dealerHoleCard = true;

    // Clear Hand Totals.
    handTotal[dealerIndex][0] = 0;
    handTotal[dealerIndex][1] = 0;
    handTotal[playerIndex][0] = 0;
    handTotal[playerIndex][1] = 0;

    // Deal initial hand.
    dealCard(playerIndex, playerHandIndex++);
    dealCard(dealerIndex, dealerHandIndex++);
    dealCard(playerIndex, playerHandIndex++);
    dealCard(dealerIndex, dealerHandIndex++);
    displayHands();

    // Dealer Blackjack?
    if (handTotal[dealerIndex][1] == 21) {
      // Yes, Display hole card.
      displayHoleCard(true);

      // Give player the bad news.
      lcd.clear();
      lcd.print("Dealer Blackjack!");

      // Player Blackjack?
      lcd.setCursor(0, 1);
      if (handTotal[playerIndex][1] == 21) {
        // Yes, Push.
        lcd.print("You Push!");
      }
      else {
        // No, Loser.
        lcd.print("You Lose ");
        lcd.print(toCurrency(bet));
        bank -= bet;
      }
    }

    // Player Blackjack?
    else if (handTotal[playerIndex][1] == 21) {
      // Yes, Show hole card.
      displayHoleCard(true);

      // Give player the good news.
      lcd.clear();
      lcd.print("Player Blackjack!");
      lcd.setCursor(0, 1);
      lcd.print("You Win ");
      lcd.print(toCurrency(bet * 1.5));
      bank += bet * 1.5;
    }

    // Play it out.
    else {
      // keyUp = Hit, keyDown = Stay.
      byte key;
      // Hit or Stay?
      do {
        lcd.setCursor(14, 0);
        lcd.print("H");
        lcd.setCursor(14, 1);
        lcd.print("S");

        // Get Up or Down key.
        key = getKey(true, true, false, false, true);
        if (key == keyUp) {
          // Hit player.
          dealCard(playerIndex, playerHandIndex++);
          displayHands();
        }
      }
      
      // Keep going until players reaches card limit, 21, busts or stays.
      while (playerHandIndex < handIndexMax && handTotal[playerIndex][0] < 21 && key != keyDown);
      
      // Player busted?
      if (handTotal[playerIndex][0] > 21) {
        // Yes, Give player the bad news.
        displayHoleCard(true);
        lcd.clear();
        lcd.print("Player Busts!");
        lcd.setCursor(0, 1);
        lcd.print("You Lose ");
        lcd.print(toCurrency(bet));
        bank -= bet;
      }
      else {
        // Play out dealer's hand.
        displayHoleCard(false);
        
        // Dealer must stand on all 17's.
        while (dealerHandIndex < handIndexMax && handTotal[dealerIndex][0] < 17 && handTotal[dealerIndex][1] < 17 ) {
          // Hit dealer.
          dealCard(dealerIndex, dealerHandIndex++);
          displayHands();
          delay(2500);
        }
        delay(1000);
        
        // Get player total.
        int playerTotal;
        if (handTotal[playerIndex][1] > 21) {
          playerTotal = handTotal[playerIndex][0];
        }
        else {
          playerTotal = handTotal[playerIndex][1];
        }
        
        // Get dealer total.
        int dealerTotal;
        if (handTotal[dealerIndex][1] > 21) {
          dealerTotal = handTotal[dealerIndex][0];
        }
        else {
          dealerTotal = handTotal[dealerIndex][1];
        }
        
        // Dealer busted?
        if (dealerTotal > 21) {
          // Yes, Give player the good news.
          lcd.clear();
          lcd.print("Dealer Busts!");
          lcd.setCursor(0, 1);
          lcd.print("You Win ");
          lcd.print(toCurrency(bet));
          bank += bet;
        }
        
        // Push?
        else if (dealerTotal == playerTotal) {
          // Yes, Give player the news.
          lcd.clear();
          lcd.print("You Push!");
        }
        
        // Highest hand?
        else {
          if (dealerTotal > playerTotal) {
            // Dealer wins.
            lcd.clear();
            lcd.print("Dealer Wins!");
            lcd.setCursor(0, 1);
            lcd.print("You Lose ");
            lcd.print(toCurrency(bet));
            bank -= bet;
          }
          else {
            // Player wins.
            lcd.clear();
            lcd.print("Player Wins!");
            lcd.setCursor(0, 1);
            lcd.print("You Win ");
            lcd.print(toCurrency(bet));
            bank += bet;
          }
        }
      }
    }
    delay(2000);
  }
  
  // Tapped out?
  if (bank <= 0) {
    // Yes, Hope you have more in the ATM.
    lcd.clear();
    lcd.print("Tapped out!");
    displayBank(1);
    delay(4000);
    lcd.clear();
    lcd.print("Visit ATM for");
    lcd.setCursor(0, 1);
    lcd.print("Another ");
    lcd.print(toCurrency(bankInit));
    bank += bankInit;
    // Wait for player to press Select.
    getKey(false, true, true, true, true);
  }
  else {
    // Broke the Casino.
    lcd.clear();
    displayBank(0);
    lcd.setCursor(0, 1);
    lcd.print("Select new table");
    bank = bankInit;
    // Wait for player to press Select.
    getKey(false, true, true, true, true);
  }
}

// Deal a card.
void dealCard(byte player, byte card) {
  // Any cards left?
  if (cardIndex >= cardIndexMax) {
    // No, Shuffle the deck.
    lcd.clear();
    lcd.print("Shuffling");
    byte cardTemp[2];
    for (byte cardShuffle = 0; cardShuffle < cardIndexMax ; cardShuffle++) {
      byte cardRandom = random(0,51);
      cardTemp[rankIndex] = deck[cardShuffle][rankIndex];
      cardTemp[suitIndex] = deck[cardShuffle][suitIndex];
      deck[cardShuffle][rankIndex] = deck[cardRandom][rankIndex];
      deck[cardShuffle][suitIndex] = deck[cardRandom][suitIndex];
      deck[cardRandom][rankIndex] = cardTemp[rankIndex];
      deck[cardRandom][suitIndex] = cardTemp[suitIndex];
    }
    cardIndex = 0;
    delay(2000);
    displayHands();
  }

  // Deal card.
  hand[player][card][rankIndex] = deck[cardIndex][rankIndex];
  hand[player][card][suitIndex] = deck[cardIndex++][suitIndex];

  // Update hand totals.
  handTotal[player][0] += min(hand[player][card][rankIndex], 10);
  handTotal[player][1] = handTotal[player][0];
  for (byte cardCount = 0; cardCount <= card; cardCount++) {
    // Found an Ace?
    if (hand[player][cardCount][rankIndex] == 1) {
      // Yes
      handTotal[player][1] += 10;
      // Only count one Ace.
      cardCount = card + 1;
    }
  }
  Serial.println("Player #" + String(player) + "\tCard #" + String(card) + "\tCard: " + String(hand[player][card][rankIndex]) + "-" + String(hand[player][card][suitIndex]));
}

// Display hands.
void displayHands() {
  // Display dealer's hand.
  lcd.clear();
  lcd.print("D:");
  displayHand(dealerIndex, dealerHandIndex);

  // Display player's hand.
  lcd.setCursor(0, 1);
  lcd.print("P:");
  displayHand(playerIndex, playerHandIndex);
}

// Display hand.
void displayHand(byte displayHandIndex, byte displayHandIndexMax) {
  // Display cards.
  for (byte card = 0; card < displayHandIndexMax; card++) {
    // Dealer Hole Card?
    if (displayHandIndex == dealerIndex && card == 0 && dealerHoleCard) {
      // Display card back.
      lcd.write((uint8_t)4);
      lcd.write((uint8_t)5);
    }
    else {
      // Display card rank.
      switch (hand[displayHandIndex][card][rankIndex]) {
      case 1:
        lcd.print("A");
        break;
      case 10:
        lcd.write((uint8_t)7);
        break;
      case 11:
        lcd.print("J");
        break;
      case 12:
        lcd.print("Q");
        break;
      case 13:
        lcd.print("K");
        break;
      default:
        lcd.print(hand[displayHandIndex][card][rankIndex]);
      }
      // Display card suit.  
      lcd.write((uint8_t)hand[displayHandIndex][card][suitIndex]);
    }
  }
}

// Display Hole Card
void displayHoleCard(boolean pause) {
      delay(2000 * pause);
      dealerHoleCard = false;
      displayHands();
      delay(2000);
}

// Display bank.
// Parameter determines LCD row.
void displayBank(byte row) {
  lcdClearRow(row);
  lcd.print("Bank ");
  lcd.print(toCurrency(bank));
}

// Get bet.
int getBet() {
  // Show player how much they have.
  displayBank(0);
  int bet = betMin;

  // keyUp = Increase bet by bet increment.
  // keyDown = Decrease bet by bet increment.
  // keySelect = Accept bet.
  byte key;
  do {
    // Prompt user.
    lcdClearRow(1);
    lcd.print("Your Bet? ");
    lcd.print(toCurrency(bet));

    // Get user input.
    key = getKey(false, true, false, false, true);
    // Increase bet if bet is less than maximum bet and less than player's bank.
    if (key == keyUp && bet < betMax && bet < bank) {
      bet += betInc;
    }
    // Decrease bet if bet is more than minimum bet.
    else if (key == keyDown && bet > betMin) {
      // Decrease bet.
      bet -= betInc; 
    }
  }
  // Keep going until user accepts bet.
  while (key != keySelect);
  return bet;
}

// Clear LCD single row.
// Leaves cursor at the beginning of the cleared row.
void lcdClearRow(byte row) {
  if (row >= 0 && row < lcdRowMax) {  
    lcd.setCursor(0, row);
    for (int x = 0; x < lcdColMax; x++) {
      lcd.print(" ");
    }
    lcd.setCursor(0, row);
  }
}

// Get keypress.
// Parameters allow any key to be masked.
byte getKey(boolean maskSelect, boolean maskLeft, boolean maskUp, boolean maskDown, boolean maskRight) {
  byte key;
  do {
    key = keypad.getKey();
  }
  while (key == 0 || key == 255 || \
    (key == keySelect && maskSelect) || \
    (key == keyLeft && maskLeft) || \
    (key == keyUp && maskUp) || \
    (key == keyDown && maskDown) || \
    (key == keyRight && maskRight));

  // Debounce keypress.
  delay(250);

  return key;
}

// Convert long to currency string.
String toCurrency(long value) {
  String dollars = String(value / 100);
  String cents = String(value % 100) + "0";
  cents = cents.substring(0,2);
  /*return "$" + dollars + "." + cents;*/
  return "$" + String(value / 100) + "." + String(String(value % 100) + "0").substring(0,2);
}

// Display splash.
void displaySplash() {
  lcd.clear();
  lcd.setCursor(3, 1);
  lcd.write((uint8_t)4);
  lcd.write((uint8_t)5);
  delay(1000);
  lcd.setCursor(7, 1);
  lcd.write((uint8_t)4);
  lcd.write((uint8_t)5);
  delay(1000);
  lcd.setCursor(3, 1);
  lcd.print("J");
  lcd.write((uint8_t)1);
  delay(1000);
  lcd.setCursor(7, 1);
  lcd.print("A");
  lcd.write((uint8_t)2);
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("Blackjack!");

  // Logarithmic blink. (Pretty fancy huh?)
  for (byte x = 15; x < 30; x++) {
    if (x % 2) {
      lcd.display();
    }
    else {
      lcd.noDisplay();
    }
    delay(pow(31 - x, 2) + 200);
  }
  delay(1000);
}


