template <typename CounterType>
void elastic(CounterType*& counters, int& w, int ratio) {
    int new_w = w / ratio;
    CounterType* new_counters = new CounterType[new_w];
    memset(new_counters, 0, sizeof(CounterType) * new_w);
    for (int j = 0; j < w; j++) {
        new_counters[j % new_w] = max(new_counters[j % new_w], counters[j]);
    }

    delete counters;
    counters = new_counters;
    w = new_w;
}
