#ifndef VOTER_H
#define VOTER_H

#include <Arduino.h>
#include <EEPROM.h>

/* =========================
   CONFIGURATION
   ========================= */

#define NUM_TEMP 3
#define NUM_CURR 3

/* ---- Fault thresholds ---- */
#define N_FAULT 3

/* ---- Agreement thresholds ---- */
#define TEMP_AGREE_DELTA     2.0f     // °C
#define TEMP_DISCARD_DELTA   6.0f

#define CURR_AGREE_DELTA     0.2f     // A
#define CURR_DISCARD_DELTA   0.6f

#define GLOBAL_TEMP_DELTA    15.0f
#define GLOBAL_CURR_DELTA    2.0f

/* =========================
   MODES
   ========================= */

#define VOTER_MODE_NOMINAL  0
#define VOTER_MODE_DEGRADED 1
#define VOTER_MODE_SAFE     2

/* =========================
   INTERNAL STRUCTS
   ========================= */

typedef struct {
  float value;
  float last_good;
  bool  valid;
  bool  faulty;
  uint8_t fault_counter;
} SensorState;

/* =========================
   INTERNAL STATE
   ========================= */

static SensorState temp[NUM_TEMP];
static SensorState curr[NUM_CURR];

static float voted_temp = 0.0f;
static float voted_curr = 0.0f;

static uint8_t temp_conf = 0;
static uint8_t curr_conf = 0;

static uint8_t voter_mode = VOTER_MODE_NOMINAL;
static bool safe_latched = false;

/* =========================
   EEPROM MAP
   ========================= */

#define EEPROM_ADDR_MODE        0
#define EEPROM_ADDR_TFAULT      1
#define EEPROM_ADDR_CFAULT      2
#define EEPROM_ADDR_TLAST       3
#define EEPROM_ADDR_CLAST       7

/* =========================
   UTILITY
   ========================= */

static float f_abs(float x) {
  return (x < 0) ? -x : x;
}

/* =========================
   INITIALIZATION
   ========================= */

static void voter_init(void) {
  for (int i = 0; i < NUM_TEMP; i++) {
    temp[i] = {0, 0, false, false, 0};
  }
  for (int i = 0; i < NUM_CURR; i++) {
    curr[i] = {0, 0, false, false, 0};
  }
}

/* =========================
   AGREEMENT ANALYSIS
   ========================= */

static void apply_agreement(
  SensorState s[3],
  float agree_delta,
  float discard_delta
) {
  float d01 = f_abs(s[0].value - s[1].value);
  float d02 = f_abs(s[0].value - s[2].value);
  float d12 = f_abs(s[1].value - s[2].value);

  if (d01 < agree_delta && d02 > discard_delta)
    s[2].fault_counter++;
  else if (d02 < agree_delta && d01 > discard_delta)
    s[1].fault_counter++;
  else if (d12 < agree_delta && d01 > discard_delta)
    s[0].fault_counter++;
}

/* =========================
   FAULT UPDATE
   ========================= */

static void update_faults(SensorState &s) {
  if (!s.valid) {
    s.fault_counter++;
  } else if (s.fault_counter > 0) {
    s.fault_counter--;
  }

  if (s.fault_counter >= N_FAULT)
    s.faulty = true;

  if (!s.faulty && s.valid)
    s.last_good = s.value;
}

/* =========================
   MODE UPDATE
   ========================= */

static void update_mode(void) {
  int valid = 0;

  for (int i = 0; i < NUM_TEMP; i++)
    if (!temp[i].faulty && temp[i].valid) valid++;

  if (valid == 3) voter_mode = VOTER_MODE_NOMINAL;
  else if (valid == 2) voter_mode = VOTER_MODE_DEGRADED;
  else {
    voter_mode = VOTER_MODE_SAFE;
    safe_latched = true;
  }
}

/* =========================
   EEPROM SAVE (SAFE ENTRY)
   ========================= */

static void save_safe_state(void) {
  EEPROM.put(EEPROM_ADDR_MODE, voter_mode);

  uint8_t tmask = 0, cmask = 0;
  for (int i = 0; i < 3; i++) {
    if (temp[i].faulty) tmask |= (1 << i);
    if (curr[i].faulty) cmask |= (1 << i);
  }

  EEPROM.put(EEPROM_ADDR_TFAULT, tmask);
  EEPROM.put(EEPROM_ADDR_CFAULT, cmask);
  EEPROM.put(EEPROM_ADDR_TLAST, voted_temp);
  EEPROM.put(EEPROM_ADDR_CLAST, voted_curr);
}

/* =========================
   VOTING
   ========================= */

static float mean_vote(SensorState s[3], uint8_t &conf) {
  float sum = 0;
  conf = 0;

  for (int i = 0; i < 3; i++) {
    if (!s[i].faulty && s[i].valid) {
      sum += s[i].value;
      conf++;
    }
  }

  if (conf > 0) return sum / conf;
  return 0;
}

/* =========================
   UPDATE ENTRY POINT
   ========================= */

static void voter_update(
  float tval[NUM_TEMP], bool tvalid[NUM_TEMP],
  float cval[NUM_CURR], bool cvalid[NUM_CURR]
) {
  if (safe_latched) return;

  for (int i = 0; i < 3; i++) {
    temp[i].value = tval[i];
    temp[i].valid = tvalid[i];
    curr[i].value = cval[i];
    curr[i].valid = cvalid[i];
  }

  if (voter_mode == VOTER_MODE_NOMINAL) {
    apply_agreement(temp, TEMP_AGREE_DELTA, TEMP_DISCARD_DELTA);
    apply_agreement(curr, CURR_AGREE_DELTA, CURR_DISCARD_DELTA);
  }

  for (int i = 0; i < 3; i++) {
    update_faults(temp[i]);
    update_faults(curr[i]);
  }

  update_mode();

  voted_temp = mean_vote(temp, temp_conf);
  voted_curr = mean_vote(curr, curr_conf);

  if (voter_mode == VOTER_MODE_SAFE)
    save_safe_state();
}

/* =========================
   GETTERS
   ========================= */

static float voter_getTemperature(void) { return voted_temp; }
static float voter_getCurrent(void)     { return voted_curr; }

static uint8_t voter_getTempConfidence(void) { return temp_conf; }
static uint8_t voter_getCurrConfidence(void) { return curr_conf; }

static uint8_t voter_getMode(void) { return voter_mode; }

static uint8_t voter_getTempFaultMask(void) {
  uint8_t m = 0;
  for (int i = 0; i < 3; i++)
    if (temp[i].faulty) m |= (1 << i);
  return m;
}

static uint8_t voter_getCurrFaultMask(void) {
  uint8_t m = 0;
  for (int i = 0; i < 3; i++)
    if (curr[i].faulty) m |= (1 << i);
  return m;
}

static bool voter_isSafeLatched(void) {
  return safe_latched;
}

#endif
