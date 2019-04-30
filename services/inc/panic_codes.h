/*
 * panic.h
 *
 *  Created on: Jan 31, 2014
 *      Author: david_s5
 */
// Add a panic code def_panic_codes(_class, led, code)
// the panic module will send SOS followed by 900 ms delay
// followed by 300 blinks of the value of code
/// ...---...    code   ...---...

def_panic_codes(Faults,RGB_COLOR_RED,HardFault)		// 1
def_panic_codes(Faults,RGB_COLOR_RED,NMIFault)		// 2
def_panic_codes(Faults,RGB_COLOR_RED,MemManage)		// 3
def_panic_codes(Faults,RGB_COLOR_RED,BusFault)		// 4
def_panic_codes(Faults,RGB_COLOR_RED,UsageFault)	// 5

def_panic_codes(Cloud,RGB_COLOR_RED,InvalidLenth)	// 6

def_panic_codes(System,RGB_COLOR_RED,Exit)			// 7
def_panic_codes(System,RGB_COLOR_RED,OutOfHeap)		// 8

def_panic_codes(System,RGB_COLOR_RED,SPIOverRun)	// 9

def_panic_codes(Software,RGB_COLOR_RED,AssertionFailure)	// 10
def_panic_codes(Software,RGB_COLOR_RED,InvalidCase)			// 11
def_panic_codes(Software,RGB_COLOR_RED,PureVirtualCall)		// 12

def_panic_codes(System,RGB_COLOR_RED,StackOverflow)			// 13

def_panic_codes(System,RGB_COLOR_RED,HeapError)	    // 14
