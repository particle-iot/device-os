struct SparkDescriptor
{
  unsigned char *num_funcs_ptr;
  const char **function_keys;
  int (*call_function)(const char *function_key, const char *arg);
  void *(*get_variable)(const char *variable_key);
};
