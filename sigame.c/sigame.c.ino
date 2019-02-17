/*
    Jeopardy! (tested for arduion atmega 2560)
    Three players and master.
    The game has three states
        - master is reading question
        - waiting for players' answers
        - one of players is answering
    The player has three states:
        - ready
        - blocked (got ban for false start)
        - asnwering
*/

//  players' input pins
#define PININPP1 10
#define PININPP2 9
#define PININPP3 8
//  master's input pins
#define PININPMQ 12  // change game state to question
#define PININPMW 11  // change game state to waiting

#define INPUTS 5  // overall of input pins

//  output pins for LED
#define LEDP1 5 // player 1 LED
#define LEDP2 4 // player 2 LED
#define LEDP3 3 // player 3 LED
#define LEDMW 6 // master 'WAIT' LED

//  GAME STATES
#define GAMEQUEST 0
#define GAMEWAIT 1
#define GAMEANS 2

//  player states
#define PLAYERREADY 0
#define PLAYERANSWER 1
#define PLAYERBLOCKED 2

//  triggers
#define QTRIGGER 0
#define WTRIGGER 1
#define P1TRIGGER 2
#define P2TRIGGER 3
#define P3TRIGGER 4

// players number
#define PLAYERS 3


//  functions 
char read_clicked();
void game_player(int);
void game_question();
void game_waiting();
void unblock_players();
void view();


// ban period for false starts (ms)
unsigned long BLOCKPERIOD = 1000;

// state of the game
int gamestate = GAMEQUEST;


//  input pins numbers
int  pins_in[INPUTS]        = {PININPMQ,  PININPMW, PININPP1, PININPP2, PININPP3};
//  pins state
//  need it for handle 'click' events, not just press
bool pins_previous[INPUTS]  = {LOW,       LOW,      LOW,      LOW,      LOW};

//  players state
int players_state[PLAYERS] = {PLAYERREADY, PLAYERREADY, PLAYERREADY};

//  time moments when players got banned
//  0 - not banned, > 0 time moment (ms) when got ban
unsigned long players_inblock_time[PLAYERS] = {0, 0, 0};
//  useful for store time in (ms)
unsigned long time;

//  useful for store input trigger number
//  example: trigger == 1 means pins_inp[1] (Master -> Wait) is clicked
int trigger;

void setup() {
//  Serial.begin(9600);
  
  for (int i = 0; i < INPUTS; i++){
    pinMode(pins_in[i], INPUT);
  }

  pinMode(LEDP1, OUTPUT);
  pinMode(LEDP2, OUTPUT);
  pinMode(LEDP3, OUTPUT);
  pinMode(LEDMW, OUTPUT);

  game_question();
  view();
}

void loop() {
  //  controller - checking for events (clicks)
  //  read clicks
  trigger = read_clicked();
  switch (trigger) {
    case QTRIGGER:
      game_question();
      break;
    
    case WTRIGGER:
      game_waiting();
      break;
    
    case P1TRIGGER: 
      game_player(P1TRIGGER);
      break;
    
    case P2TRIGGER: 
      game_player(P2TRIGGER);
      break; 
    
    case P3TRIGGER: 
      game_player(P3TRIGGER);
      break;
  }
  //  unblock (if possible) players
  unblock_players();
  //  render view (LEDS ON/OFF)
  view();
//  delay(500);
}

void view(){
    // prints table - trigger # : gamestate # : p1 state : p2 state : p3 state 
//  Serial.print(trigger);
//  Serial.print(" ");
//  Serial.print(gamestate);
//  Serial.print(" ");
//  for (int i = 0; i < PLAYERS; i++){
//    Serial.print(players_state[i]);
//    Serial.print(" ");  
//  }
//  Serial.println();
  switch (gamestate) {
    case GAMEQUEST:
      digitalWrite(LEDMW, LOW);
      digitalWrite(LEDP1, LOW);
      digitalWrite(LEDP2, LOW);
      digitalWrite(LEDP3, LOW);
      break;

    case GAMEWAIT:
      digitalWrite(LEDMW, HIGH);
      digitalWrite(LEDP1, LOW);
      digitalWrite(LEDP2, LOW);
      digitalWrite(LEDP3, LOW);
      break;
    
    case GAMEANS:
      digitalWrite(LEDMW, LOW);
      digitalWrite(LEDP1, players_state[0] == PLAYERANSWER ? HIGH : LOW);
      digitalWrite(LEDP2, players_state[1] == PLAYERANSWER ? HIGH : LOW);
      digitalWrite(LEDP3, players_state[2] == PLAYERANSWER ? HIGH : LOW);
      break;
  }
}

/*
  char read_clicked()

  Returns trigger number 0--4.
  Multiple one-moment clicks are ignored, returns only first in pins_in[] order.
  The click is change of pin's state from LOW to HIGH.
*/
char read_clicked(){
  int reading;
  int clicked = -1;
  bool found = false;
  // read pins now
  // write 'now' value to previous
  // remember first clicked 
  for (int i = 0; i < INPUTS; i++){
    reading = digitalRead( pins_in[i] );
    if (reading == HIGH && pins_previous[i] == LOW && !found){
      clicked = i;
      found = true;
    }
    pins_previous[i] = reading;
  }
  return clicked;
}

/*
  void game_question()

  Moves game to QUESTION STATE.
  Actually behave as resetting of game and players state.
*/
void game_question(){
  gamestate = GAMEQUEST;
  // ??? memcpy? 
  //  all inputs to LOW (because LOW -> HIGH is a click)
  for (int i = 0; i < INPUTS; i++){
    pins_previous[i] = LOW;
  }
  //  all players to READY
  for (int i = 0; i < PLAYERS; i++){
    players_state[i] = PLAYERREADY;
  }
  //  inblock time is 0 for all players
  for (int i = 0; i < PLAYERS; i++){
    players_inblock_time[i] = 0;  
  }
};

/*
  void game_waiting()

  Moves game to WAIT state.
*/
void game_waiting(){
  // when someone answering - return its state to PLAYERREADY
  if (gamestate == GAMEANS){
    for (int i = 0; i < PLAYERS; i++){
      if (players_state[i] == PLAYERANSWER){
        players_state[i] = PLAYERREADY;
        break;
      } 
    }
  }
  gamestate = GAMEWAIT;
};

/*
  void game_player(int ptrigger)

  Handle player's click. 
  Game can change its state to ANSWER.
  Some players can get ban.
  
  # Arguments
  int ptrigger == 2, 3, 5: player's trigger number in pins_in[]
*/
void game_player(int ptrigger){
  // actual player's number
  int p = ptrigger - 2;  // 0--2
  if (players_state[p] == PLAYERREADY){
    switch (gamestate) {
      case GAMEWAIT:
        // click is correct
        gamestate = GAMEANS;
        players_state[p] = PLAYERANSWER;
        break;
      case GAMEQUEST:
        // ban, player clicked when master reading question
        players_state[p] = PLAYERBLOCKED;
        players_inblock_time[p] = millis();
        break;
      case GAMEANS:
        // ban, player clicked when another player is answering
        players_state[p] = PLAYERBLOCKED;
        players_inblock_time[p] = millis();
        break;
    }
  } else if (players_state[p] == PLAYERBLOCKED){
      //  when blocked player clicks again
      //  it gets new ban
      players_inblock_time[p] = millis(); 
  }
  //  when players is answering it can clicks as many as it want
};

/*
  void unblock_players()
  
  Unblock blocked players if they blocked for time > BLOCKPERIOD.
*/
void unblock_players(){
  time = millis();
  for (int i = 0; i < PLAYERS; i++){
    if (players_inblock_time[i] > 0 && (time - players_inblock_time[i] >= BLOCKPERIOD)){
      // unban
      players_inblock_time[i] = 0;
      players_state[i] = PLAYERREADY;
    }
  }
}
