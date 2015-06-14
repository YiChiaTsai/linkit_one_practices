#ifndef TESTER_DEFS_H
#define TESTER_DEFS_H

// DC is short for Data Channel
typedef enum{ // !!! shoulde be started from 0 !!!
  DC_ID_START = 0,
  
  DC_ID_SWITCH_C = DC_ID_START,
  DC_ID_SWITCH,
  DC_ID_CATEGORY_C,
  DC_ID_CATEGORY,
  DC_ID_INTEGER_C,
  DC_ID_INTEGER,
  DC_ID_FLOAT_C,
  DC_ID_FLOAT,
  DC_ID_HEX_C,
  DC_ID_HEX,
  DC_ID_STRING_C,
  DC_ID_STRING,
  DC_ID_GPS_C,
  DC_ID_GPS,
  DC_ID_GPIO_C,
  DC_ID_GPIO,
  DC_ID_PWM_C,
  DC_ID_PWM,
  
  DC_ID_MAX
}DC_ID_ENUM;

typedef enum{
  DC_TYPE_START, 
  
  DC_TYPE_SWITCH = DC_TYPE_START,
  DC_TYPE_CATEGORY,
  DC_TYPE_INTEGER,
  DC_TYPE_FLOAT,
  DC_TYPE_HEX,
  DC_TYPE_STRING,
  DC_TYPE_GPS,
  DC_TYPE_GPIO,
  DC_TYPE_PWM,
  
  DC_TYPE_MAX,
}DC_TYPE;

typedef struct{
  const char *id;
  DC_TYPE type;
}DC_ID_TYPE_STRUCT;

#endif // TESTER_DEFS_H
