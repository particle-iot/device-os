unsigned int __heap_start;
void *__brkval;

/*
 * The free list structure as maintained by the 
 * avr-libc memory allocation routines.
 */
struct __freelist {
  size_t sz;
  struct __freelist *nx;
};

/* The head of the free list structure */
struct __freelist *__flp;

// MemoryFree library based on code posted here:
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1213583720/15
// 
// Extended by Matthew Murdoch to include walking of the free list.

//#ifdef __cplusplus
    //extern "C" {
//#endif

int freeMemory();

//#ifdef __cplusplus
    //}
//#endif

/* Calculates the size of the free list */
int freeListSize() {
  struct __freelist* current;
  int total = 0;

  for (current = __flp; current; current = current->nx) {
    total += 2; /* Add two bytes for the memory block's header  */
    total += (int) current->sz;
  }

  return total;
}

int freeMemory() {
  int free_memory;

  if ((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__heap_start);
  } else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
    free_memory += freeListSize();
  }
  return free_memory;
}

// On Spark Core
//
// Reported free memory with str commented out:
// 17584 bytes RAM
//
// Reported free memory with str and Serial.println(str) uncommented:
// 17552 bytes RAM
//
// Difference: 32 bytes (31 ascii chars + null terminator)

// 14-bytes string
//char str[] = "Hello, Spark!!! Hello, Spark!!!";
int free_mem;
void setup() {
    Serial1.begin(115200);
    Spark.variable("free_mem", &free_mem, INT);
}


void loop() {
    //Serial1.println(str);
    free_mem = freeMemory();
    Serial.print("freeMemory()=");
    Serial.println(freeMemory());

    delay(1000);
}
