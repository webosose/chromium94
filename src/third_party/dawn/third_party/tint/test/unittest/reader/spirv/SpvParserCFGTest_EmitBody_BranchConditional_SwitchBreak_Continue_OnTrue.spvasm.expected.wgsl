var<private> var_1 : u32;

fn main_1() {
  var_1 = 1u;
  loop {
    var_1 = 2u;
    var_1 = 3u;
    switch(42u) {
      case 40u: {
        var_1 = 40u;
        if (false) {
          continue;
        }
      }
      default: {
      }
    }
    var_1 = 6u;

    continuing {
      var_1 = 7u;
    }
  }
  var_1 = 8u;
  return;
}

[[stage(fragment)]]
fn main() {
  main_1();
}
