/*	WKURIList.h
	Copyright 2001, Apple, Inc. All rights reserved.
*/

#import "WKURIList.h"
#import "WebKitReallyPrivate.h"

struct WKURIListNode
{
    unsigned hash;
    WKURIEntry *entry;
    WKURIListNode *prev;
    WKURIListNode *next;
};

static WKURIListNode *newURIListNode(WKURIEntry *entry)
{
    WKURIListNode *node;

    [entry retain];
    
    node = malloc(sizeof(WKURIListNode));
    node->hash = [entry hash];
    node->entry = entry;
    node->prev = nil;
    node->next = nil;
    
    return node;    
}

static void freeNode(WKURIListNode *node)
{
    // it is important to autorelase here rather than using 
    // a straight release since we often return an entry
    // as a result of an operation that causes a node
    // to be freed
    [node->entry autorelease];
    free(node);    
}


@implementation WKURIList

-(id)init
{
    if (self != [super init])
    {
        return nil;
    }
    
    _head = _tail = nil;
    _count = 0;
    _maximumSize = -1;
    _allowsDuplicates = NO;
    
    return self;
}

-(void)dealloc
{
    WKURIListNode *curNode;
    WKURIListNode *delNode;

    curNode = _head;

    while (curNode) {
        delNode = curNode;
        curNode = curNode->next;
        freeNode(delNode);
    }
}

-(BOOL)allowsDuplicates
{
    return _allowsDuplicates;
}

-(void)setAllowsDuplicates:(BOOL)allowsDuplicates
{
    _allowsDuplicates = allowsDuplicates;
}

-(int)count
{
    return _count;
}

-(int)maximumSize
{
    return _maximumSize;
}

-(void)setMaximumSize:(int)size
{
    WEBKIT_ASSERT(size > 0 || size == -1);
    _maximumSize = size;
}



-(WKURIEntry *)addURL:(NSURL *)url withTitle:(NSString *)title;
{
    WKURIEntry *result;
    
    result = [[WKURIEntry alloc] initWithURL:url title:title];
    [self addEntry:result];
    
    return result;
}

-(void)addEntry:(WKURIEntry *)entry
{
    WKURIListNode *node;
    unsigned hash;

    if (!_allowsDuplicates) {
        // search the list first and remove any entry with the same URL
        // having the same title does not count
        // use the hash codes of the urls to speed up the linear search
        hash = [entry hash];
        for (node = _head; node != nil; node = node->next) {
            if (hash == node->hash && [entry isEqual:node->entry]) {
                _count--;
                if (node == _head) {
                    _head = node->next;
                    if (_head) {
                        _head->prev = nil;
                    }
                }
                else if (node == _tail) {
                    _tail = node->prev;
                    if (_tail) {
                        _tail->next = nil;
                    }
                }
                else {
                    node->prev->next = node->next;
                    node->next->prev = node->prev;
                }
                freeNode(node);
                break;
            }
        }
    }

    // make new entry and put it at the head of the list
    node = newURIListNode(entry);
    node->next = _head;
    _head = node;
    _count++;
    if (_count == 1) {
        _tail = _head;
    }
    
    if (_maximumSize != -1 && _count > _maximumSize) {
        // drop off the tail
        node = _tail;
        _tail = _tail->prev;
        _tail->next = nil;
        freeNode(node);
        _count--;
    }
}

-(WKURIEntry *)removeURL:(NSURL *)url
{
    WKURIEntry *removedEntry;
    WKURIListNode *node;
    unsigned hash;
    
    removedEntry = nil;
    hash = [url hash];

    for (node = _head; node != nil; node = node->next) {
        if (hash == node->hash && [url isEqual:[node->entry url]]) {
            _count--;
            removedEntry = node->entry;
            if (node == _head) {
                _head = node->next;
                if (_head) {
                    _head->prev = nil;
                }
            }
            else if (node == _tail) {
                _tail = node->prev;
                if (_tail) {
                    _tail->next = nil;
                }
            }
            else {
                node->prev->next = node->next;
                node->next->prev = node->prev;
            }
            freeNode(node);
            break;
        }
    }
    
    return removedEntry;
}

-(BOOL)removeEntry:(WKURIEntry *)entry
{
    BOOL removed;
    WKURIListNode *node;
    unsigned hash;
    
    removed = NO;
    hash = [entry hash];

    for (node = _head; node != nil; node = node->next) {
        if (hash == node->hash && [entry isEqual:node->entry]) {
            _count--;
            removed = YES;
            if (node == _head) {
                _head = node->next;
                if (_head) {
                    _head->prev = nil;
                }
            }
            else if (node == _tail) {
                _tail = node->prev;
                if (_tail) {
                    _tail->next = nil;
                }
            }
            else {
                node->prev->next = node->next;
                node->next->prev = node->prev;
            }
            freeNode(node);
            break;
        }
    }
    
    return removed;
}

-(WKURIEntry *)entryForURL:(NSURL *)url
{
    WKURIEntry *foundEntry;
    WKURIListNode *node;
    unsigned hash;
    
    foundEntry = nil;
    hash = [url hash];

    for (node = _head; node != nil; node = node->next) {
        if (hash == node->hash && [url isEqual:[node->entry url]]) {
            foundEntry = node->entry;
            break;
        }
    }
    
    return foundEntry;
}

-(WKURIEntry *)entryAtIndex:(int)index
{
    int i;
    WKURIListNode *node;

    WEBKIT_ASSERT(index >= 0 && index < _count);

    node = _head;

    for (i = 0; i < index; i++) {
        node = node->next;
    }

    return node->entry;    
}

-(WKURIEntry *)removeEntryAtIndex:(int)index
{
    WKURIEntry *removedEntry;
    WKURIListNode *node;
    int i;

    WEBKIT_ASSERT(index > 0 && index < _count);

    node = _head;

    for (i = 0; i < index; i++) {
        node = node->next;
    }

    _count--;
    removedEntry = node->entry;
    if (node == _head) {
        _head = node->next;
        if (_head) {
            _head->prev = nil;
        }
    }
    else if (node == _tail) {
        _tail = node->prev;
        if (_tail) {
            _tail->next = nil;
        }
    }
    else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    freeNode(node);

    return removedEntry;
}

-(void)removeEntriesToIndex:(int)index
{
    WKURIListNode *node;
    WKURIListNode *delNode;
    int i;

    WEBKIT_ASSERT(index > 0 && index < _count);

    node = _head;

    for (i = 0; i < index; i++) {
        delNode = node;
        node = node->next;
        freeNode(delNode);
        _count--;
    }
    
    _head = node;
}

@end
