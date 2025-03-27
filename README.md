# os_design
# 开发环境
Linux环境，建议ubuntu
建议使用gitpod：https://gitpod.io/flex-or-classic ， 便于搭建环境

# 使用方法
1. 将该项目克隆到本地
```
git clone https://github.com/shucan-yang/os_design.git
```
2. 配置环境
```
sudo apt update
sudo apt install build-essential
```
3. 编译运行
```
cd os_design
make clean
make
./build/BUPTscsOS
```
# 开发流程建议
预备知识：git 的使用方法，
入门操作可以参考教程 https://liaoxuefeng.com/books/git/introduction/index.html
可以参考这个可视化git操作的小游戏来直观地学习git https://learngitbranching.js.org/?demo=&locale=zh_CN

## 开发准备
需要检查自己配置的远程仓库：
```
git remote -v
```
如果结果如下，则已经配置好了（这是在git clone时就会帮你自动配置的，如果不是用git clone而是下载的zip，就需要手动配置）
```
origin  https://github.com/shucan-yang/os_design.git (fetch)
origin  https://github.com/shucan-yang/os_design.git (push)
```
手动配置方法：
```
git remote add origin https://github.com/shucan-yang/os_design.git
```

## 开发流程
1. 切换到main分支：
首先查看自己目前的分支情况：
```
git branch
```
如果当前不是在main分支上（即不是“*main”的情况），就要切换到main分支：
```
git checkout main
```
再查看自己的分支情况：
```
git branch
```
如果结果类似于：
```
gitpod /workspace/os_design (main) $ git branch
* main
```
就说明在main上了

2. 拉取最新更新：这会保证你的main分支是最新的
```
git pull
```

3. 分支更新：
如果你此前没有创建自己的分支，那么执行命令创建新分支并切换到自己的分支。分支名可以取自己的名字的英文拼音，便于识别，例如：
```
git checkout -b shucanyang
```
**不要照着打命令，现在已经有shucanyang这个分支了，请自己创建自己的分支**

如果你已经有了自己的分支，那么执行以下命令合并main分支上可能有的新提交：
```
git merge main
```

4. 进行具体开发：这一步你会修改文件，创建文件等

5. 提交修改：
```
git add . 
git commit -m "这里写你这次提交新增了什么修改等描述性文字"
```

6. 推送到远程仓库：将**自己的分支**推送出去，如下以shucanyang分支为例。这一步要求你已经完成了“开发准备”里的配置
```
git push origin shucanyang
```


7. 审核并合并：在github的仓库处创建PR/MR，然后合并到main分支上

8. 回到main分支，拉取main最新更新：
```
git checkout main
git pull
```

9. 删除之前自己创建的分支，并在最新的main上创建新的分支：
```
git branch -d shucanyang
git checkout -b shucanyang
```

10. 后续开发：回到步骤4即可。