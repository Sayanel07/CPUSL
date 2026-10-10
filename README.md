
# CPUSL

----------
CPUSL (Central Processing Unit Shader Language) is a high level programing language built in c++ that allows you to operate on a given array of pixels using material acceleration techniques such as SIMD. It comes with a simple c++ interface that allows you to run CPUSL code with an external program. It is strongly typed and statements have to end with the `;` token.

## Categories
Your code can be written under 3 categories:

### `CONST:`
The constant category is where you define constants. You can initialize float1-4 constants and should assign their value in this category. Once the constant is declared, you cannot change their value.

### `ENTRY:`
The entry category is where you declare your entry variables. These variables cannot be set from inside the shader, and has to be given as argument when calling the interface's execute function.

### `PS:`
The PS (or Pixel Shader) category is where you compute your pixel's color. The code in this category will be executed once per pixel.

> **Note:** You cannot write code outside of a category.

## Types


### `float`
### `float2`
### `float3`
### `float4`
32-bit floating point numerical values.
When writing vector literals, you have to pass through the constructor, as such:
`float2(4, 2)`
`float4(0, 1, 2, 3)`

### `texture1d`
### `texture2d`
Textures of 1/2 dimension(s). You can only declare a texture in the ENTRY category, and cannot write on a texture inside the shader.

----------

## Declaring a Variable
Variables are strongly typed, which means the type must be specified before declaring a variable.

**Declare with initial value:**

```
float4 color_correction = (0.1, 0, 0.3, 0);
```
Declares "color_correction " and sets its components to `0.1`, `0`, `0.3`, and `0` respectively.

----------

## System Variable

You cannot set system variables, except for `pixel_color`.
You are expected to set this variable as the output color of your shader.

### `pixel_color`

`float4`
The in/out RGBA color of the current pixel. 
This variable is the only system variable that can be set. 

### `pixel_coord`

`float2`
The pixel coordinates (x, y) of the current pixel

### `uv_coord`

`float2`
The uv coordinates (u, v) of the current pixel.

### `pi`

`float`
The pi constant (`3.1415927`).

### `source`

`texture2d`
The initial texture over which the PS category operates.

----------

## Accessor

float2/3/4 variables and constants' components are accessible by using the .x/.y/.z/.w accessors.

**Example:**

```
float4 variable = float4(1, 2, 3, 4);
pixel_color.x = variable.y;
```
----------

## Operators

### `a = b`

Assignement
> **Note:** You can only assign two values of the same type.

### `a + b`

Addition
> **Note:** You can only add two values of the same type.

### `a - b`

Subtraction
> **Note:** You can only subtract values of the same type.

### `a * b`

Multiplication
> **Note:** You can multiply values of the same type or a vector with a scalar.

### `a / b`

Division
> **Note:** You can divide a value by another value of the same type or a vector by a scalar.

----------

## Functions

Every function only returns a value and is not transformative. If you do not use the value returned by a function, the function will have no effect.

#### `min`
Arguments: 
(float) a
(float) b

Return value:
(float) The lowest value between a and b.

#### `max`
Arguments: 
(float) a
(float) b

Return value:
(float) The highest value between a and b.

#### `clamp`
Arguments: 
(float) value: The value compared with min/max.
(float) min: The minimum return value.
(float) max: The maximum return value.

Return value:
(float) The value, clamped between min and max.

#### `lerp`
Arguments: 
(float) a
(float) b
(float) t

Return value:
(float) The linear interpolation of a and b by t.

#### `sqrt`

Arguments: 
(float) value: The value of which you want the square root.

Return value:
(float) The square root of value.

#### `cos`
Arguments: 
(float) value: The value of which you want the cosine.

Return value:
(float) The cosine of value.

#### `sin`
Arguments: 
(float) value: The value of which you want the sine.

Return value:
(float) The sine of value.

#### `tan`
Arguments: 
(float) value: The value of which you want the tangent.

Return value:
(float) The tangent of value.

#### `length`
Arguments: 
(float2/3/4) vector

Return value:
(float) The length/norm of vector.

#### `lengthsqr`
Arguments: 
(float2/3/4) vector

Return value:
(float) The length/norm squared of vector.


#### `normalize`
Arguments: 
(float2/3/4) vector

Return value:
(float2/3/4) The normalized vector (divided by its norm).

#### `cross_product`
Arguments: 
(float3) a
(float3) b

Return value:
(float3) The cross product of a and b.

#### `dot_product`
Arguments: 
(float2/3/4) a
(float2/3/4) b

Return value:
(float) The dot product of a and b.

#### `sample`
Arguments: 
(texture1d/2d) tex
(float/float2) pixel_coordinates

Return value:
(float4) The color of tex at pixel_coordinates.

#### `size`
Arguments: 
(texture1d/2d) tex

Return value:
(float/float2) The width/height of tex in pixels.

----------

## Example

```
ENTRY:
float zoom_factor;
texture2d tex;
PS:
float2 compressed_coord = pixel_coord * (size(tex) / size(source));
float2 distorted_coord = lerp(compressed_coord, size(tex)/2, sqrt(zoom_factor));
pixel_color = sample(tex, distorted_coord);
```
