.ifdef SPARK_INIT_STARTUP
/* Call the hal core configuration function. */
	bl  HAL_Core_Config
/* Call C++ constructors. */
	bl  CallConstructors
.endif
