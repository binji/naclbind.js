const void* foo(void) {
  static int x;
  return &x;
}
