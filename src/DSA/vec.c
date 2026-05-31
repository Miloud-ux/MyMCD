#include "vec.h"

struct Vector {
        int *data;
        size_t len;
        size_t count;
};

iVector *vectorCreate(size_t size) {
    iVector *self = calloc(1, sizeof(iVector));
    if (!self) {
        return NULL;
    }

    self->len = size > 0 ? size : 1;
    self->count = 0;
    self->data = calloc(self->len, sizeof(int));

    if (!self->data) {
        free(self);
        return NULL;
    }
    return self;
}

void vectorPrint(iVector *self) {
    if (!self || !self->data) {
        return;
    }
    printf("{\n");
    for (size_t i = 0; i < self->count; i++) {
        printf("{Data at [%zu]: %d}\n", i, self->data[i]);
    }
    printf("}\n");
}

void vectorPush(iVector *self, int data) {
    if (!self) {
        return;
    }

    if (self->count >= self->len) {
        size_t newlen = self->len * 2;
        int *temp = realloc(self->data, newlen * sizeof(int));
        if (!temp) {
            printf("Error Allocating memory\n");
            return;
        }
        self->data = temp;

        size_t addedElements = newlen - self->len;
        memset(&(self->data[self->len]), 0, addedElements * sizeof(int));
        self->len = newlen;
    }

    self->data[self->count] = data;
    self->count++;
}

void vectorPop(iVector *self) {
    if (!self || !self->data || self->count == 0) {
        return;
    }

    int del = self->data[self->count - 1];
    self->count--;
    self->data[self->count] = 0;

    if (self->count < (size_t)(self->len / 2) && self->len > 10) {
        size_t newlen = (size_t)(self->len / 2);
        int *temp = realloc(self->data, sizeof(int) * newlen);
        if (!temp) {
            printf("Shrinking vector failed\n");
            return;
        }
        self->data = temp;
        self->len = newlen;
    }

    printf("Deleted item : %d at index: %zu\n", del, self->count);
}

void vectorFree(iVector *self) {
    if (!self) {
        return;
    }
    if (self->data) {
        free(self->data);
    }
    free(self);
}

int vectorGet(iVector *self, size_t index) {
    if (!self || !self->data || index >= self->count) {
        return -1;
    }
    return self->data[index];
}
