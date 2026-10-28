#define MinTemp -10
#define MaxTemp 70
#define MaxGradTemp 5
#define MinCurr -2
#define MaxCurr 0.3
#define MaxGradCurr 0.5
#define MaxPen 30

int len;
float vec[3];
int faulty[3];
float old_vec[3];

int fault_handling(int* faulty, int len, int indx1, int indx2){
  indx1 = 0; indx2 = 0;
  if (len == 3) for (int i=0; i<len; i++){
    if (faulty[i] > MaxPen-1){
      indx1 = i + 1;
      for (int k=i+1; k<len; k++){
        if (faulty[k]){
          indx2 = k + 1;
          EnterSafeMode(indx1, indx2);
          break;
        }
      }
      break;
    }
  }
  else if (len == 2) for (int i=0; i<len; i++){
    if (fauty[i] > MaxPen-1){
      indx1 = i+1;
      EnterSafeMode(indx1, indx2);
    }
  }

  if (indx1) mode = 1;
  return mode;

}


void setup(){

}

void loop(){
  if (len == 3){
    for (int i=0; i<len; i++){
      if (vec[i] > MaxTemp && faulty[i] < MaxPen){
        faulty[i] = faulty[i] + 2;
      }
      if (vec[i] < MinTemp && faulty[i] < MaxPen){
        faulty[i] = faulty[i] + 2;
      }
      if (fabs(vec[i] - old_vec[i]) > MaxGradTemp && faulty[i] < MaxPen){
        faulty[i] = faulty[i] + 2;
      }
      else if (faulty[i]){
        faulty[i] = faulty[i] - 1;
      }
    }
  }
  else if (len == 2){
    for (int i=0; i<len; i++){
      if (vec[i] > MaxCurr && faulty[i] < MaxPen){
        faulty[i] = faulty[i] + 2;
      }
      if (vec[i] < MinCurr && faulty[i] < MaxPen){
        faulty[i] = faulty[i] + 2;
      }
      if (fabs(vec[i] - old_vec[i]) > MaxGradCurr && faulty[i] < MaxPen){
        faulty[i] = faulty[i] + 2;
      }
      else if (faulty[i]){
        faulty[i] = faulty[i] - 1;
      }
    }
  }

  // Voting di Tommy

  }

}