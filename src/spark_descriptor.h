struct SparkDescriptor
{
  unsigned char *num_funcs_ptr;
  const char **function_keys;
  int (*call_function)(unsigned char *function_key, unsigned char *arg);
};
