function my(a) print(a..'非匿名函数') end 
my('这是')


(function (m) print(m) end)('廖丽辉成功了！')

(function (m) my(m) end)('廖丽辉成功了！')