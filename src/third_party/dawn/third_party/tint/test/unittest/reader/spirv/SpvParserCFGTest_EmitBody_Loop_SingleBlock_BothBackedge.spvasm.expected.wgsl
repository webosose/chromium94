var<private> var_1 : u32;

fn main_1() {
  var_1 = 0u;
  loop {
    var_1 = 1u;
  }
  var_1 = 999u;
  return;
}

[[stage(fragment)]]
fn main() {
  main_1();
}
