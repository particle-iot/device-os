
/*
Copyright (c) 2014 Matt Paine

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "application.h"
#include "unit-test.h"

FakeStreamBuffer::FakeStreamBuffer() : _firstNode(NULL), _lastNode(NULL) {
}

FakeStreamBuffer::~FakeStreamBuffer() {
}

void FakeStreamBuffer::reset() {
    _clearBuffer();
    FakeStream::reset();
}

void FakeStreamBuffer::setToEndOfStream() {
    _pushByte(-1);
}

void FakeStreamBuffer::nextByte(byte b) {
    _pushByte(b);
}

void FakeStreamBuffer::nextBytes(const char *s) {
    int next = 0;
    while (s[next] != 0) {
        nextByte((byte)s[next]);
        next++;
    }
}

int FakeStreamBuffer::available()  {
    return _getBufferSize(-1);
}

int FakeStreamBuffer::read() {
    if (_firstNode != NULL) {
        int ret = (int)_nextByte();
        if (ret == 255) {
            ret = -1;
        }
        return ret;
    }
    return -1;
}

int FakeStreamBuffer::peek() {
  if (_firstNode != 0) {
    return _firstNode->value;
  }
  return -1;
}

BufferNode* FakeStreamBuffer::_createNode (byte value) {
    BufferNode *node = (BufferNode*)malloc(sizeof(BufferNode));
    node->value = value;
    node->next = NULL;
    return node;
}

void FakeStreamBuffer::_appendNode(BufferNode *node) {
    if (_firstNode == NULL) {
        // This is the only node!
        _firstNode = node;
        _lastNode = node;
    }
    else {
        _lastNode->next = node;
        _lastNode = node;
    }
}

void FakeStreamBuffer::_pushByte (byte value) {
    BufferNode *node = _createNode (value);
    _appendNode(node);
}

BufferNode* FakeStreamBuffer::_popNode() {
    BufferNode *node = _firstNode;
    if (node != NULL) {
        _firstNode = node->next;
    }
    return node;
}

byte FakeStreamBuffer::_nextByte() {
    byte ret = -1;
    BufferNode *node = _popNode();
    if (node != NULL) {
        ret = node->value;
        free(node);
    }
    return ret;
}

int FakeStreamBuffer::_getBufferSize () {
    return _getBufferSize(-1);
}
int FakeStreamBuffer::_getBufferSize (byte stopAt) {
    int count = 0;
    BufferNode *node = _firstNode;
    while (node != 0 && node->value != stopAt) {
        count++;
        node = node->next;
    }
    return count;
}

void FakeStreamBuffer::_clearBuffer() {
    BufferNode *node = _firstNode;
    BufferNode *nextNode;
    while (node != 0) {
        nextNode = node->next;
        free(node);
        node = nextNode;
    }
    _firstNode = NULL;
    _lastNode = NULL;
}

