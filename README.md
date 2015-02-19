# Postfix mruby database plugin

## Require

### Postfix

Postfix 3.0 with `dynamicmaps=yes`. See [Postfix Installtion From Source Code](http://www.postfix.org/INSTALL.html#dynamicmaps_enable).

### mruby

To build `libmruby.so`:

```
% git clone https://github.com/mruby/mruby.git
% cd mruby
% patch -p1 < mruby.diff
% make
% mkdir -p $MRUBYDIR
% cp ./build/host/lib/libmruby.so $MRUBYDIR
```

mruby.diff is [here](https://github.com/tmtm/postfix-mruby/blob/master/mruby.diff).

## Install

```
% git clone https://github.com/tmtm/postfix-mruby.git
% cd postfix-mruby
% gcc -I $POSTFIX_SRC_DIR/include -I $MRUBY_WORK_DIR/include -fPIC -g -O -DLINUX3 -c postfix-mruby.c
% LD_RUN_PATH=$MRUBYDIR gcc -shared -o postfix-mruby.so postfix-mruby.o -L$MRUBYDIR -lmruby -lm
% sudo cp postfix-mruby.so $(postconf -h shlib_directory)
```

## Usage

Write mruby script that returns an object with `#lookup` method.

The `#lookup` method takes one argument and returns String.

Use `mruby:fullpath_of_the_script` as lookup table.

## Example

[upcase.rb]
```ruby
class Upcase
  def lookup(key)
    return key.upcase
  end
end
Upcase.new
```

```
% postmap -q abc mruby:/path/to/upcase.rb
ABC
```

## License

[MIT license](http://opensource.org/licenses/mit-license.php)
