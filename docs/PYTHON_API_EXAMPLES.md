# Corona Engine Python API 使用指南

**版本**: 1.0  
**日期**: 2025-01-14

本文档提供了 Corona Engine Python API 的完整使用示例和最佳实践。

---

## 目录

1. [基础概念](#基础概念)
2. [Geometry - 几何体](#geometry---几何体)
3. [组件系统](#组件系统)
4. [Actor 和 Profile](#actor-和-profile)
5. [Camera 和 Viewport](#camera-和-viewport)
6. [Scene 场景管理](#scene-场景管理)
7. [完整示例](#完整示例)
8. [最佳实践](#最佳实践)

---

## 基础概念

### 核心设计理念

Corona Engine API 采用纯 OOP 设计：

- **Geometry**: 几何体，作为所有组件的锚点
- **组件**: Optics（渲染）、Mechanics（物理）、Kinematics（动画）、Acoustics（声音）
- **Actor**: 实体，可以拥有多个 Profile（组件配置集合）
- **Scene**: 场景，管理 Actor、Viewport、Environment
- **生命周期**: Python 负责所有对象的生命周期管理

---

## Geometry - 几何体

Geometry 是所有组件的基础，存储模型数据和变换信息。

### 基本使用

```python
from corona_engine import Geometry

# 创建几何体
geometry = Geometry("assets/model/character.obj")

# 设置变换
geometry.set_position([0.0, 0.0, 0.0])
geometry.set_rotation([0.0, 1.57, 0.0])  # 欧拉角（ZYX 顺序）
geometry.set_scale([1.0, 1.0, 1.0])

# 获取变换
pos = geometry.get_position()
rot = geometry.get_rotation()
scale = geometry.get_scale()

print(f"Position: {pos}")
print(f"Rotation: {rot}")
print(f"Scale: {scale}")
```

### 变换操作

```python
# 移动对象
geometry.set_position([10.0, 0.0, 5.0])

# 旋转对象（欧拉角，单位：弧度）
import math
geometry.set_rotation([0.0, math.radians(90), 0.0])  # 绕 Y 轴旋转 90 度

# 缩放对象
geometry.set_scale([2.0, 2.0, 2.0])  # 放大 2 倍
```

---

## 组件系统

每个组件都依赖于 Geometry，提供不同的功能。

### Optics - 渲染组件

```python
from corona_engine import Optics

# 创建渲染组件
geometry = Geometry("character.obj")
optics = Optics(geometry)

# Optics 自动处理渲染，无需额外配置
# 如果模型有骨骼，会自动创建蒙皮数据
```

### Mechanics - 物理组件

```python
from corona_engine import Mechanics

# 创建物理组件
geometry = Geometry("box.obj")
mechanics = Mechanics(geometry)

# Mechanics 自动计算碰撞盒
# 可以通过 geometry 访问
print(f"Geometry: {mechanics.get_geometry()}")
```

### Kinematics - 动画组件

```python
from corona_engine import Kinematics

# 创建动画组件
geometry = Geometry("animated_character.obj")
kinematics = Kinematics(geometry)

# 设置动画
kinematics.set_animation(0)  # 选择第 0 个动画

# 播放动画
kinematics.play_animation(1.0)  # 速度 1.0

# 停止动画
kinematics.stop_animation()

# 查询动画状态
current_anim = kinematics.get_animation_index()
current_time = kinematics.get_current_time()
```

### Acoustics - 声学组件

```python
from corona_engine import Acoustics

# 创建声学组件
geometry = Geometry("sound_source.obj")
acoustics = Acoustics(geometry)

# 设置音量
acoustics.set_volume(0.8)

# 获取音量
volume = acoustics.get_volume()
```

---

## Actor 和 Profile

Actor 是游戏中的实体，可以拥有多个 Profile（组件配置集合）。

### 基本 Actor

```python
from corona_engine import Actor

# 创建 Actor
actor = Actor()

# 创建 Profile
profile = Actor.Profile()

# 添加组件到 Profile
geometry = Geometry("character.obj")
profile.optics = Optics(geometry)
profile.mechanics = Mechanics(geometry)
profile.kinematics = Kinematics(geometry)
profile.acoustics = Acoustics(geometry)

# 添加 Profile 到 Actor
actor.add_profile(profile)

# 设置为激活 Profile
actor.set_active_profile(profile)

# 获取激活的 Profile
active = actor.get_active_profile()
if active:
    print(f"Active profile has optics: {active.optics is not None}")
```

### 多 Profile 切换

```python
# 创建多个 Profile 用于不同状态
actor = Actor()

# Profile 1: 正常状态
geometry1 = Geometry("character_normal.obj")
profile_normal = Actor.Profile()
profile_normal.optics = Optics(geometry1)
profile_normal.mechanics = Mechanics(geometry1)
actor.add_profile(profile_normal)

# Profile 2: 隐形状态
geometry2 = Geometry("character_invisible.obj")
profile_stealth = Actor.Profile()
profile_stealth.optics = Optics(geometry2)
# 注意：没有 mechanics，隐形时不参与物理碰撞
actor.add_profile(profile_stealth)

# 切换状态
actor.set_active_profile(profile_normal)   # 正常
actor.set_active_profile(profile_stealth)  # 隐形

# 移除 Profile
actor.remove_profile(profile_stealth)

# 查询 Profile 数量
count = actor.profile_count()
```

### 组件复用

```python
# 多个 Profile 可以共享组件
geometry = Geometry("character.obj")

# 创建共享的组件
shared_mechanics = Mechanics(geometry)
shared_acoustics = Acoustics(geometry)

# Profile 1: 近距离（高精度）
geometry_high = Geometry("character_high.obj")
profile_near = Actor.Profile()
profile_near.optics = Optics(geometry_high)
profile_near.mechanics = shared_mechanics  # 共享物理
profile_near.acoustics = shared_acoustics  # 共享声音

# Profile 2: 远距离（低精度）
geometry_low = Geometry("character_low.obj")
profile_far = Actor.Profile()
profile_far.optics = Optics(geometry_low)
profile_far.mechanics = shared_mechanics   # 共享物理
profile_far.acoustics = shared_acoustics   # 共享声音

actor = Actor()
actor.add_profile(profile_near)
actor.add_profile(profile_far)

# 根据距离切换
distance = 15.0
if distance < 10.0:
    actor.set_active_profile(profile_near)
else:
    actor.set_active_profile(profile_far)
```

---

## Camera 和 Viewport

### Camera 基本使用

```python
from corona_engine import Camera

# 创建 Camera
camera = Camera()

# 设置 Camera 参数
position = [0.0, 5.0, 10.0]
forward = [0.0, 0.0, -1.0]
world_up = [0.0, 1.0, 0.0]
fov = 60.0

camera.set(position, forward, world_up, fov)

# 获取 Camera 参数
pos = camera.get_position()
fwd = camera.get_forward()
up = camera.get_world_up()
fov = camera.get_fov()

# 设置渲染表面
camera.set_surface(surface_id)
```

### Viewport 基本使用

```python
from corona_engine import Viewport

# 创建 Viewport
viewport = Viewport(1920, 1080)

# 设置 Camera
camera = Camera()
viewport.set_camera(camera)

# 检查是否有 Camera
if viewport.has_camera():
    cam = viewport.get_camera()
    cam.set([0, 5, 10], [0, 0, -1], [0, 1, 0], 60)

# 设置 Viewport 大小
viewport.set_size(1920, 1080)

# 获取 Viewport 属性
width = viewport.get_width()
height = viewport.get_height()
aspect = viewport.get_aspect_ratio()

# 移除 Camera
viewport.remove_camera()
```

### Camera 复用

```python
# 多个 Viewport 可以共享同一个 Camera
camera = Camera()
camera.set([0, 10, 20], [0, 0, -1], [0, 1, 0], 60)

# 主视口
main_viewport = Viewport(1920, 1080)
main_viewport.set_camera(camera)

# 小地图（共享相同的 Camera）
minimap_viewport = Viewport(200, 200)
minimap_viewport.set_camera(camera)

# 通过任何一个 Viewport 修改 Camera，其他 Viewport 都会同步
main_viewport.get_camera().set([0, 20, 30], [0, -1, -1], [0, 1, 0], 45)
```

### 图像效果

```python
from corona_engine import ImageEffects

# 创建图像效果
effects = ImageEffects()

# 添加到 Viewport
viewport.set_image_effects(effects)

# 检查是否有图像效果
if viewport.has_image_effects():
    fx = viewport.get_image_effects()
    # 使用 fx...

# 移除图像效果
viewport.remove_image_effects()
```

---

## Scene 场景管理

### 基本场景

```python
from corona_engine import Scene, Environment

# 创建场景
scene = Scene()

# 设置环境
environment = Environment()
environment.set_sun_direction([1.0, -1.0, 0.0])
environment.set_floor_grid(True)
scene.set_environment(environment)

# 添加 Actor
actor = Actor()
# ... 配置 actor ...
scene.add_actor(actor)

# 添加 Viewport
viewport = Viewport(1920, 1080)
# ... 配置 viewport ...
scene.add_viewport(viewport)

# 查询
print(f"Actor count: {scene.actor_count()}")
print(f"Viewport count: {scene.viewport_count()}")

# 检查
if scene.has_actor(actor):
    print("Actor is in scene")

# 移除
scene.remove_actor(actor)
scene.remove_viewport(viewport)

# 清空
scene.clear_actors()
scene.clear_viewports()
```

---

## 完整示例

### 示例 1: 简单的 3D 场景

```python
from corona_engine import (
    Scene, Environment, Actor, Geometry,
    Optics, Mechanics, Camera, Viewport
)

class SimpleScene:
    def __init__(self):
        # 创建场景
        self.scene = Scene()
        
        # 环境
        self.environment = Environment()
        self.environment.set_sun_direction([1.0, -1.0, 0.5])
        self.scene.set_environment(self.environment)
        
        # Camera 和 Viewport
        self.camera = Camera()
        self.camera.set([0, 5, 10], [0, 0, -1], [0, 1, 0], 60)
        
        self.viewport = Viewport(1920, 1080)
        self.viewport.set_camera(self.camera)
        self.scene.add_viewport(self.viewport)
        
        # 创建一个 Actor
        self.player = self.create_player()
        self.scene.add_actor(self.player)
    
    def create_player(self):
        actor = Actor()
        
        # 创建 Geometry
        geometry = Geometry("assets/model/player.obj")
        geometry.set_position([0, 0, 0])
        
        # 创建 Profile
        profile = Actor.Profile()
        profile.optics = Optics(geometry)
        profile.mechanics = Mechanics(geometry)
        
        actor.add_profile(profile)
        actor.set_active_profile(profile)
        
        return actor

# 使用
game_scene = SimpleScene()
```

### 示例 2: 角色动画系统

```python
from corona_engine import (
    Actor, Geometry, Optics, Kinematics
)

class AnimatedCharacter:
    def __init__(self, model_path):
        self.geometry = Geometry(model_path)
        self.geometry.set_position([0, 0, 0])
        
        # 创建组件
        self.optics = Optics(self.geometry)
        self.kinematics = Kinematics(self.geometry)
        
        # 创建 Actor
        self.actor = Actor()
        
        # 创建 Profile
        profile = Actor.Profile()
        profile.optics = self.optics
        profile.kinematics = self.kinematics
        
        self.actor.add_profile(profile)
        self.actor.set_active_profile(profile)
        
        # 动画状态
        self.ANIM_IDLE = 0
        self.ANIM_WALK = 1
        self.ANIM_RUN = 2
        self.ANIM_JUMP = 3
    
    def play_idle(self):
        self.kinematics.set_animation(self.ANIM_IDLE)
        self.kinematics.play_animation(1.0)
    
    def play_walk(self):
        self.kinematics.set_animation(self.ANIM_WALK)
        self.kinematics.play_animation(1.0)
    
    def play_run(self):
        self.kinematics.set_animation(self.ANIM_RUN)
        self.kinematics.play_animation(1.5)
    
    def play_jump(self):
        self.kinematics.set_animation(self.ANIM_JUMP)
        self.kinematics.play_animation(1.0)
    
    def stop(self):
        self.kinematics.stop_animation()
    
    def move_to(self, x, y, z):
        self.geometry.set_position([x, y, z])

# 使用
character = AnimatedCharacter("assets/model/hero.obj")
character.play_walk()
character.move_to(10, 0, 5)
```

### 示例 3: LOD (细节层次) 系统

```python
class LODCharacter:
    def __init__(self):
        # 创建不同精度的 Geometry
        self.geometry_high = Geometry("character_high.obj")
        self.geometry_medium = Geometry("character_medium.obj")
        self.geometry_low = Geometry("character_low.obj")
        
        # 共享物理和动画
        self.mechanics = Mechanics(self.geometry_high)
        self.kinematics = Kinematics(self.geometry_high)
        
        # 创建 Actor
        self.actor = Actor()
        
        # Profile 1: 高精度 (< 10m)
        self.profile_high = Actor.Profile()
        self.profile_high.optics = Optics(self.geometry_high)
        self.profile_high.mechanics = self.mechanics
        self.profile_high.kinematics = self.kinematics
        self.actor.add_profile(self.profile_high)
        
        # Profile 2: 中精度 (10-30m)
        self.profile_medium = Actor.Profile()
        self.profile_medium.optics = Optics(self.geometry_medium)
        self.profile_medium.mechanics = self.mechanics
        self.profile_medium.kinematics = self.kinematics
        self.actor.add_profile(self.profile_medium)
        
        # Profile 3: 低精度 (> 30m)
        self.profile_low = Actor.Profile()
        self.profile_low.optics = Optics(self.geometry_low)
        self.profile_low.mechanics = self.mechanics
        # 远距离不需要动画
        self.actor.add_profile(self.profile_low)
        
        # 默认使用高精度
        self.actor.set_active_profile(self.profile_high)
    
    def update_lod(self, distance):
        """根据距离更新 LOD"""
        if distance < 10.0:
            self.actor.set_active_profile(self.profile_high)
        elif distance < 30.0:
            self.actor.set_active_profile(self.profile_medium)
        else:
            self.actor.set_active_profile(self.profile_low)

# 使用
lod_character = LODCharacter()
camera_distance = 15.0
lod_character.update_lod(camera_distance)
```

### 示例 4: 游戏场景管理器

```python
class GameSceneManager:
    def __init__(self):
        # 场景
        self.scene = Scene()
        
        # 环境
        self.environment = Environment()
        self.environment.set_sun_direction([1, -1, 0])
        self.scene.set_environment(self.environment)
        
        # Camera 池
        self.cameras = {
            'main': Camera(),
            'debug': Camera(),
            'cutscene': Camera()
        }
        
        # Viewport
        self.main_viewport = Viewport(1920, 1080)
        self.main_viewport.set_camera(self.cameras['main'])
        self.scene.add_viewport(self.main_viewport)
        
        # Actor 管理
        self.actors = {}
        self.next_actor_id = 0
    
    def add_actor(self, name, model_path):
        """添加 Actor 到场景"""
        geometry = Geometry(model_path)
        
        actor = Actor()
        profile = Actor.Profile()
        profile.optics = Optics(geometry)
        profile.mechanics = Mechanics(geometry)
        
        actor.add_profile(profile)
        actor.set_active_profile(profile)
        
        self.actors[name] = actor
        self.scene.add_actor(actor)
        
        return actor
    
    def remove_actor(self, name):
        """从场景移除 Actor"""
        if name in self.actors:
            actor = self.actors[name]
            self.scene.remove_actor(actor)
            del self.actors[name]
    
    def get_actor(self, name):
        """获取 Actor"""
        return self.actors.get(name)
    
    def switch_camera(self, camera_name):
        """切换 Camera"""
        if camera_name in self.cameras:
            self.main_viewport.set_camera(self.cameras[camera_name])
    
    def cleanup(self):
        """清理场景"""
        self.scene.clear_actors()
        self.scene.clear_viewports()
        self.actors.clear()

# 使用
manager = GameSceneManager()

# 添加玩家
player = manager.add_actor('player', 'player.obj')
player_geo = player.get_active_profile().optics.get_geometry()
player_geo.set_position([0, 0, 0])

# 添加敌人
for i in range(10):
    enemy = manager.add_actor(f'enemy_{i}', 'enemy.obj')
    enemy_geo = enemy.get_active_profile().optics.get_geometry()
    enemy_geo.set_position([i * 2, 0, 5])

# 切换到调试 Camera
manager.switch_camera('debug')

# 清理
manager.cleanup()
```

---

## 最佳实践

### 1. 生命周期管理

```python
class ResourceManager:
    """集中管理所有对象的生命周期"""
    
    def __init__(self):
        self.geometries = []
        self.actors = []
        self.cameras = []
        self.viewports = []
        self.components = []
    
    def create_geometry(self, path):
        geo = Geometry(path)
        self.geometries.append(geo)
        return geo
    
    def create_actor(self):
        actor = Actor()
        self.actors.append(actor)
        return actor
    
    def create_camera(self):
        camera = Camera()
        self.cameras.append(camera)
        return camera
    
    def cleanup(self):
        """清理所有资源"""
        self.actors.clear()
        self.viewports.clear()
        self.cameras.clear()
        self.components.clear()
        self.geometries.clear()
```

### 2. 组件复用

```python
class ComponentPool:
    """组件池，用于复用组件"""
    
    def __init__(self):
        self.optics_pool = {}
        self.mechanics_pool = {}
        self.kinematics_pool = {}
    
    def get_or_create_optics(self, geometry):
        key = id(geometry)
        if key not in self.optics_pool:
            self.optics_pool[key] = Optics(geometry)
        return self.optics_pool[key]
    
    def get_or_create_mechanics(self, geometry):
        key = id(geometry)
        if key not in self.mechanics_pool:
            self.mechanics_pool[key] = Mechanics(geometry)
        return self.mechanics_pool[key]
```

### 3. 错误处理

```python
def safe_set_camera(viewport, camera):
    """安全地设置 Camera"""
    if viewport is None:
        print("Error: viewport is None")
        return False
    
    if camera is None:
        print("Warning: camera is None, removing camera from viewport")
        viewport.remove_camera()
        return True
    
    viewport.set_camera(camera)
    return True

def safe_get_active_profile(actor):
    """安全地获取激活的 Profile"""
    if actor is None:
        return None
    
    profile = actor.get_active_profile()
    if profile is None:
        print(f"Warning: Actor has no active profile")
    
    return profile
```

### 4. 调试辅助

```python
def debug_print_actor(actor):
    """打印 Actor 信息"""
    print(f"Actor Profile Count: {actor.profile_count()}")
    
    profile = actor.get_active_profile()
    if profile:
        print("Active Profile Components:")
        print(f"  Optics: {profile.optics is not None}")
        print(f"  Mechanics: {profile.mechanics is not None}")
        print(f"  Kinematics: {profile.kinematics is not None}")
        print(f"  Acoustics: {profile.acoustics is not None}")
        
        if profile.optics:
            geo = profile.optics.get_geometry()
            pos = geo.get_position()
            print(f"  Position: {pos}")

def debug_print_scene(scene):
    """打印场景信息"""
    print(f"Scene Info:")
    print(f"  Actors: {scene.actor_count()}")
    print(f"  Viewports: {scene.viewport_count()}")
    print(f"  Has Environment: {scene.has_environment()}")
```

---

## 常见问题

### Q: 如何确保对象不被提前销毁？

**A**: 在 Python 中保持对对象的引用

```python
# ✅ 正确：在类中保持引用
class Game:
    def __init__(self):
        self.camera = Camera()
        self.viewport = Viewport()
        self.viewport.set_camera(self.camera)

# ❌ 错误：对象会被销毁
def bad_example():
    camera = Camera()
    viewport.set_camera(camera)
    # camera 超出作用域，被销毁！
```

### Q: 如何实现组件的复用？

**A**: 多个 Profile 可以共享同一个组件实例

```python
mechanics = Mechanics(geometry)

profile1 = Actor.Profile()
profile1.mechanics = mechanics

profile2 = Actor.Profile()
profile2.mechanics = mechanics  # 复用
```

### Q: Camera 可以被多个 Viewport 使用吗？

**A**: 可以，多个 Viewport 可以共享同一个 Camera

```python
camera = Camera()
viewport1.set_camera(camera)
viewport2.set_camera(camera)
```

---

## 总结

Corona Engine Python API 的核心特点：

1. **纯 OOP 设计**: 所有操作都通过对象方法
2. **组件化**: Geometry + 组件系统
3. **灵活的 Profile 系统**: 支持多套组件配置
4. **对象复用**: 组件和 Camera 可以被多个对象共享
5. **Python 管理生命周期**: 简单直观的资源管理

**开始你的 Corona Engine 之旅吧！** 🚀

