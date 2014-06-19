uint32_t freeMemoryAvailable(void)
{
  extern char _end;
  char *current_heap_end = &_end;
  char *current_stack_pointer = (char *)__get_MSP();
  return (current_stack_pointer - current_heap_end);
}

int free_mem;
void setup() {
  Spark.variable("free_mem", &free_mem, INT);
}

void loop() {
 free_mem = freeMemoryAvailable();
 delay(100);
}
