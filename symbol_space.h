#ifndef SYMBOL_SPACE_H
#define SYMBOL_SPACE_H
#include "symbol.h"
#include <stdexcept>
#include <stdint.h>

namespace symbol {

template<typename Value>
class Space {
    class Node {
    public:
        Node* next;
        Symbol key;
        Value value;
        Node(Symbol k, Value v): next(NULL), key(k), value(v) {}
    };

    Node* head;
public:

    // New, empty space
	Space(): head(NULL) {}
	~Space(){
        Node* prev = head;
        while ( prev ) {
            Node* next = prev->next;
            delete prev;
            prev = next;
        }
    }

    // returns a pointer to the Value, or NULL if it 
    // isn't found in the space.
    Value* get(Symbol key) {
        Node* next = head;
        while ( next != NULL ) {
            if ( key == next->key ) {
                return &next->value;
            } else if ( key < next->key ) {
                // list is kept sorted by id, so if we haven't found it yet
                // we're not going to.
                return NULL;
            }
            next = next->next;
        }
        // exhausted the list, not found
        return NULL;
    }

    void set(Symbol key, Value value) {
        if ( head == NULL ) {
            // short circuit case of empty Space
            head = new Node(key, value);
            return;
        } else {
            // special case for when we need to insert before head
            if ( key < head->key ) {
                Node* old_head = head;
                head = new Node(key, value);
                head->next = old_head;
            }

            Node* prev = head;
            Node* next = prev->next;
            while ( prev != NULL ) {
                if ( prev->key == key ) {
                    // replace the value
                    prev->value = value;
                    return;
                } else if ( next == NULL ) {
                    // add to the end
                    prev->next = new Node(key, value);
                    return;
                } else if ( key > prev->key && key < next->key ) {
                    // insert it between prev and next - this keeps the list sorted
                    Node* new_node = new Node(key, value);
                    prev->next = new_node;
                    new_node->next = next;
                    return;
                }
                // advance
                prev = next;
                next = next->next;
            } // end while
        }
    }

    void del(Symbol key) {
        // short circuit special cases so the general case is straightforward.
        if ( head == NULL ) {
            return;
        } else if ( head->key == key ) {
            Node* next = head->next; // sometimes next will be NULL; that's fine.
            delete head;
            head = next;
            return;
        }

        Node* prev = head;
        Node* next = head->next;

        while ( next != NULL ) {
            if ( next->key == key ) {
                // skip the node; also handles the end of the list just fine. :)
                prev->next = next->next;
                delete next;
                return;
            }
            // advance
            prev = next;
            next = next->next;
        }
    }
};

}

#endif
