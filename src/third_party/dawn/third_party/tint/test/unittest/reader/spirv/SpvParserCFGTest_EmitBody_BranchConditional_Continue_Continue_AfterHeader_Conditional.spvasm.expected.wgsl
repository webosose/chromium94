var<private> var_1 : u32;

fn main_1() {
  var_1 = 0u;
  loop {
    var_1 = 1u;
    var_1 = 2u;
    if (true) {
      var_1 = 3u;
      continue;
    }
    var_1 = 4u;

    continuing {
      var_1 = 5u;
    }
  }
  var_1 = 6u;
  return;
}

[[stage(fragment)]]
fn main() {
  main_1();
}
