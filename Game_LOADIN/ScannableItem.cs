using UnityEngine;

public class ScannableItem : MonoBehaviour
{
    [Header("物品信息")]
    public string itemName = "未命名物品";
    [TextArea(3, 10)]
    public string itemDescription = "这里写详细介绍...";

    [Header("UI漂浮偏移量 (Y越大字越高)")]
    public Vector3 uiOffset = new Vector3(0, 0.5f, 0);

    // 【内部变量】标记是否已经被收集过了
    [HideInInspector]
    public bool isCollected = false;
}