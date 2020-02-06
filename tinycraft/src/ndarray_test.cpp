TEST(NDArray, Basic) {
    int data[15];
    for (int i = 0; i < NUM_ELEMS(data); i++) {
        data[i] = i;
    }
    auto arr = NDArray<int, 2>::create_from_data(data, {5, 3});
    ASSERT_EQ(arr.get({1, 1}), 4);
    auto w = arr.window({1, 1}, {1, 1});
    ASSERT_EQ(w.get({0, 0}), 4);
    ASSERT_EQ(w.get({-1, -1}), 0);
    ASSERT_EQ(w.get({1, 1}), 8);
    w.fill(0);
    for (int i = 0; i < NUM_ELEMS(data); i++) {
        if (i == 4) {
            ASSERT_EQ(data[i], 0);
        } else {
            ASSERT_EQ(data[i], i);
        }
    }
}
