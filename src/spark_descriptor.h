struct SparkDescriptor
{
  int (*num_functions)(void);
  void (*copy_function_key)(char *destination, int function_index);
  int (*call_function)(const char *function_key, const char *arg);

  void *(*get_variable)(const char *variable_key);

  bool (*was_ota_upgrade_successful)(void);
  void (*ota_upgrade_status_sent)(void);
};
