var<private> var_1 : u32;

fn main_1() {
  var_1 = 1u;
  loop {
    var_1 = 2u;
    var_1 = 3u;
    switch(42u) {
      case 40u: {
        var_1 = 4u;
        continue;
      }
      default: {
      }
    }
    var_1 = 5u;

    continuing {
      var_1 = 6u;
    }
  }
  var_1 = 7u;
  return;
}

[[stage(fragment)]]
fn main() {
  main_1();
}
