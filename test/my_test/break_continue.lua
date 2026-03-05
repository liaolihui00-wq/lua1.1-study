print("for循环")
for i = 1, 10, 1 do
    if i >= 3 and i <= 8 then
        print(i)
    end
end

print("\nwhile循环")
i = 1
while i <= 10 do
    if i < 3 then
        i = i + 1  
    elseif i > 8 then
        break
    else
        print(i)
        i = i + 1 
    end
end
print("\nrepeat循环")
i = 1
repeat
    i = i + 1 
    if i >= 3 and i <= 8 then
        print(i)
    end
until i > 8
